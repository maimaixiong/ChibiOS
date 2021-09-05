/*
 * branch : minisys
 * board  : RT-STM32F407-CGW
 * author : dinglx@uincore.net (mmx)
 * rtos   : ChibiOS
*/

#include <stdio.h>
#include <string.h>

#include "ch.h"
#include "hal.h"
#include "shell.h"
#include "chprintf.h"

#include "usbcfg.h"
#include "my.h"
#include "dbc/vw.h"

int log_level = 6;

static can_obj_vw_h_t vw_obj;
static uint8_t hca_stat=0;
static uint8_t tsk_stat=0;
static double wheelSpeeds_fl=0;
static double wheelSpeeds_fr=0;
static double wheelSpeeds_rl=0;
static double wheelSpeeds_rr=0;
static double vEgoRaw=0;
static bool stop_still=false;
static bool acc_enable=false;
static bool hca_err=false;

mutex_t mtx;
volatile bool LaneAssist = false;


/*===========================================================================*/
/* Command line related.                                                     */
/*===========================================================================*/

#define SHELL_WA_SIZE   THD_WORKING_AREA_SIZE(4096)

/* Can be measured using dd if=/dev/xxxx of=/dev/null bs=512 count=10000.*/
static void cmd_write(BaseSequentialStream *chp, int argc, char *argv[]) {
      static uint8_t buf[] =
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
        
        (void)argv;
        if (argc > 0) {
                chprintf(chp, "Usage: write\r\n");
                    return;
                      
        }

        while (chnGetTimeout((BaseChannel *)chp, TIME_IMMEDIATE) == Q_TIMEOUT) {
        #if 1
            /* Writing in channel mode.*/
           chnWrite(&SDU1, buf, sizeof buf - 1);
        #else
           /* Writing in buffer mode.*/
           (void) obqGetEmptyBufferTimeout(&SDU1.obqueue, TIME_INFINITE);
           memcpy(SDU1.obqueue.ptr, buf, SERIAL_USB_BUFFERS_SIZE);
           obqPostFullBuffer(&SDU1.obqueue, SERIAL_USB_BUFFERS_SIZE);
        #endif
                                      
        }
        chprintf(chp, "\r\n\nstopped\r\n");
          
}

static void cmd_loglevel(BaseSequentialStream *chp, int argc, char *argv[]) {

       if (argc == 0) {
           chprintf(chp, "loglevel:%d\r\n", log_level);
           return;
       }
       
       if (argc == 1) {
           log_level = asc2nibble(argv[0][0]);
           chprintf(chp, "loglevel:%0d\r\n", log_level);
           return;
       }

       chprintf(chp, "\r\n");
}

static void cmd_ot(BaseSequentialStream *chp, int argc, char *argv[]) {

       if (argc == 0) {
           chprintf(chp, "ot:%u\r\n", ot);
           return;
       }
       
       if (argc == 1) {
           //ot = asc2nibble(argv[0][0]);
           ot = atoi(argv[0]);
           chprintf(chp, "ot:%0d\r\n", ot);
           return;
       }

       chprintf(chp, "\r\n");
}

static void cmd_hackmode(BaseSequentialStream *chp, int argc, char *argv[]) {

       if (argc == 0) {
           chprintf(chp, "hackmode:%d\r\n", hack_mode);
           return;
       }
       
       if (argc == 1) {
           hack_mode = asc2nibble(argv[0][0])>0;
           chprintf(chp, "hackmode:%0d\r\n", hack_mode);
           return;
       }

       chprintf(chp, "\r\n");
}


static void cmd_candump(BaseSequentialStream *chp, int argc, char *argv[]) {

    (void)chp;
    (void)argc;
    (void)argv;

    myRxMsg_t CanMsg;

    while(true) {
        if(getMailMessage(&CanMsg)==0)
         candump(&CanMsg);
    }

}

static void cmd_test(BaseSequentialStream *chp, int argc, char *argv[]) {

    int i;

    int base;

    (void)chp;
    (void)argc;
    (void)argv;

    CANRxFrame f;

    uint8_t d0,d1;
    uint16_t w;


    for(base=0; base<10; base++) {

        log(6, "------------------ base %d ------------------------\n\r", base);

        for(i=0; i<8; i++){
            f.data8[i] = i+base;
        }
        
        vw_crc_init();
        log(6, "crc: %02x ", vw_crc(f.data64[0], 8));

        //log(6, "uint64_t %016x\n\r", f.data64[0]);
        //log(6, "uint64_t %lx\n\r",   f.data64[0]);
        //log(6, "uint64_t %08x %08x\n\r", f.data64[0], f.data64[0]>>32 );
        //log(6, "uint32_t %08x %08x\n\r", f.data32[1], f.data32[0]);
        log(6, "uint8_t  %02x %02x %02x %02x %02x %02x %02x %02x\n\r", 
                f.data8[0], 
                f.data8[1], 
                f.data8[2], 
                f.data8[3], 
                f.data8[4], 
                f.data8[5], 
                f.data8[6], 
                f.data8[7]);
	 d0= 0x34;
	 d1= 0x12;

	 w =  (d0 + ((uint16_t)(d1&0x3f)<<8) ) ;
	 log(6, "0x1234 =  %08x  0x12 = %02x\n\r", w,  (w>>8)&0x3f ); 
	  
    }

}

static void cmd_test1(BaseSequentialStream *chp, int argc, char *argv[]) {

    (void)chp;
    (void)argc;
    (void)argv;

    int i = 0;
    
    while(1) {
        log(6, "\033[2J hello world! %08d %10u\n\r", i++, chVTGetSystemTimeX() );
        chThdSleep(10000);
        log(6, " hello world! %08d %10u\n\r", i++, chVTGetSystemTimeX() );
        chThdSleep(10000);
        log(6, " hello world! %08d %10u\n\r", i++, chVTGetSystemTimeX() );
        chThdSleep(10000);
        log(6, " hello world! %08d %10u\n\r", i++, chVTGetSystemTimeX() );
        chThdSleep(10000);
        log(6, " hello world! %08d %10u\n\r", i++, chVTGetSystemTimeX() );
        chThdSleep(10000);
        log(6, " hello world! %08d %10u\n\r", i++, chVTGetSystemTimeX() );
        chThdSleep(10000);
        log(6, " hello world! %08d %10u\n\r", i++, chVTGetSystemTimeX() );
        chThdSleep(10000);
        log(6, " hello world! %08d %10u\n\r", i++, chVTGetSystemTimeX() );
        chThdSleep(10000);
        log(6, " hello world! %08d %10u\n\r", i++, chVTGetSystemTimeX() );
        chThdSleep(10000);
        log(6, " hello world! %08d %10u\n\r", i++, chVTGetSystemTimeX() );
        chThdSleep(10000);
        log(6, " hello world! %08d %10u\n\r", i++, chVTGetSystemTimeX() );
        chThdSleep(1000000);
    }
}


static void cmd_canmsg(BaseSequentialStream *chp, int argc, char *argv[]) {

       unsigned long id;       

       if (argc == 0) {
           chprintf(chp, "Usage: canmsg canid(dec)\r\n");
           return;
       }
       
       if (argc == 1) {
           id  = atoi(argv[0]);
           print_message(&vw_obj, id, chp);
           return;
       }

       chprintf(chp, "\r\n");
    
}

static void cmd_led(BaseSequentialStream *chp, int argc, char *argv[]) {

       int led_id;       
       int val;

       if (argc != 2) {
           chprintf(chp, "Usage: led led_id(0:blue 1:green 2:red)  0/1(off/on)\r\n");
           return;
       }
       
       led_id  = atoi(argv[0]);
       val  = atoi(argv[1]);

       switch(led_id){
	       case 0:
	       default:
		       if(val==0)
			       palClearLine(LINE_LED_BLUE);
		       else
			       palSetLine(LINE_LED_BLUE);
		       break;
	       case 1:
		       if(val==0)
			       palClearLine(LINE_LED_GREEN);
		       else
			       palSetLine(LINE_LED_GREEN);
		       break;
	       case 3:
		       if(val==0)
			       palClearLine(LINE_LED_RED);
		       else
			       palSetLine(LINE_LED_RED);
		       break;


       }

       chprintf(chp, "\r\n");
    
}

static void cmd_caninfo(BaseSequentialStream *chp, int argc, char *argv[]) {

    (void)argc;
    (void)argv;

    chprintf(chp, "CAN Info: rx=%d,%d tx=%d:%d,%d:%d err=%d,%d LaneAssist=%d(%d:%d:%d) \n\r"
            , can_rx_cnt[0],  can_rx_cnt[1]
            , can_txd_cnt[0], can_tx_cnt[0]
            , can_txd_cnt[1], can_tx_cnt[1]
            , can_err_cnt[0], can_err_cnt[1]
            , LaneAssist, hca_err, acc_enable, stop_still
            );
}


extern void cmd_cansend(BaseSequentialStream *chp, int argc, char *argv[]);

static const ShellCommand commands[] = {
      {"write", cmd_write},
      {"loglevel", cmd_loglevel},
      {"candump", cmd_candump},
      {"test", cmd_test},
      {"hackmode", cmd_hackmode},
      {"ot", cmd_ot},
      {"test1", cmd_test1},
      {"canmsg", cmd_canmsg},
      {"cansend", cmd_cansend},
      {"caninfo", cmd_caninfo},
      {"led", cmd_led},
      {NULL, NULL}
      
};

#define HISTORY_SIZE 64*16
static char shell_history_buffer[HISTORY_SIZE];
static char* shell_completion_buffer[HISTORY_SIZE];

static const ShellConfig shell_cfg = {
      (BaseSequentialStream *)&SDU1,
       commands,
       shell_history_buffer,
       HISTORY_SIZE,
       shell_completion_buffer
};


/*===========================================================================*/
/* Generic code.                                                             */
/*===========================================================================*/

/*
 *  Green LED blinker thread, times are in milliseconds.
 *  
 */
static THD_WORKING_AREA(waThread1, 1024);
static THD_FUNCTION(Thread1, arg) {

      (void)arg;
      
      chRegSetThreadName("blinker");

      while (true) {

        systime_t time = serusbcfg.usbp->state == USB_ACTIVE ? 100 : 500;
        palClearLine(LINE_LED_GREEN);
        chThdSleepMilliseconds(time);
        palSetLine(LINE_LED_GREEN);
        chThdSleepMilliseconds(time);

      }
        
}


int tx_err_cnt = 0;

static THD_WORKING_AREA(waThread_ProcessData, 2048);
static THD_FUNCTION(Thread_ProcessData, arg) {

    myRxMsg_t CanMsg;
    CANTxFrame  txFrame;
    CANDriver *to_fwd;
    int to_fwd_num;
    int ret;
    
    (void)arg;
    
    chRegSetThreadName("ProcessData");

    while(true) {

        if(getMailMessage(&CanMsg)==0){
            if( CanMsg.can_bus>0
                && CanMsg.can_bus<=2
            )
            {
		if(CanMsg.fr.IDE == 0) {
		    //log(7, "RX: %d, %03x, %d : %08x %08x\n\r", CanMsg.can_bus, CanMsg.fr.SID, CanMsg.fr.DLC, CanMsg.fr.data32[0], CanMsg.fr.data32[1]);
                    ret = unpack_message(&vw_obj, CanMsg.fr.SID, CanMsg.fr.data64[0], CanMsg.fr.DLC, CanMsg.timestamp);

                    if (ret == 0){

                       hca_stat = vw_obj.can_0x09f_LH_EPS_03.EPS_HCA_Status;
                       hca_err = ( hca_stat==0 || hca_stat==1 || hca_stat==2 || hca_stat==4  );

                       tsk_stat = vw_obj.can_0x120_TSK_06.TSK_Status;
                       acc_enable = ( tsk_stat==3 || tsk_stat==4 || tsk_stat==5  );

                       decode_can_0x0b2_ESP_VL_Radgeschw_02(&vw_obj, &wheelSpeeds_fl);
                       decode_can_0x0b2_ESP_VR_Radgeschw_02(&vw_obj, &wheelSpeeds_fr);
                       decode_can_0x0b2_ESP_HL_Radgeschw_02(&vw_obj, &wheelSpeeds_rl);
                       decode_can_0x0b2_ESP_HR_Radgeschw_02(&vw_obj, &wheelSpeeds_rr);
                       vEgoRaw = (wheelSpeeds_fl + wheelSpeeds_fr + wheelSpeeds_rl + wheelSpeeds_rr)/4;
                       stop_still = (vEgoRaw<1);
		       chMtxLock(&mtx);                       
                       LaneAssist = ( !hca_err && acc_enable && !stop_still );
		       chMtxUnlock(&mtx);
                    }
                }
                
                canframe_copy(&txFrame, &CanMsg.fr);

                switch( CanMsg.can_bus ){
                    case 1:
                        to_fwd = &CAND2;
			to_fwd_num = 2;
                        break;
                    case 2:
                        to_fwd = &CAND1;
			to_fwd_num = 1;
                        break;
                    default: 
                        to_fwd = NULL;
		   	log(LOG_LEVEL_ERR, "FORWARD CAN BUS %d ERROR!\n\r", CanMsg.can_bus);
                        break;
                }
                
                if (to_fwd == NULL) continue;

                if (CanMsg.can_bus == 2 && CanMsg.fr.SID==0x126 && CanMsg.fr.IDE==0) {
                   hca_process(&txFrame);
                }

                can_tx_cnt[to_fwd_num-1]++;

                if( canTransmit(to_fwd, CAN_ANY_MAILBOX, &txFrame, TIME_MS2I(100)) != MSG_OK  ){
                    tx_err_cnt++;
                    log(LOG_LEVEL_ERR, "canTransmit: CAN%d ID=%x X=%d R=%d L=%d (tx_err_count=%d)\n\r", to_fwd_num, txFrame.EID, txFrame.EID, txFrame.RTR, txFrame.DLC, tx_err_cnt);
                } 
                else {
                    can_txd_cnt[CanMsg.can_bus-1]++;
                }

            }

        }

    }
        
}

/*
 * Application entry point.
 */
int main(void) {

  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  chSysInit();

  /*
   * Activates the serial driver 2 using the driver default configuration.
   * PD5(TX) and PD6(RX) are routed to USART2.
   */
  sdStart(&SD2, NULL);
  palSetPadMode(GPIOD, 5, PAL_MODE_ALTERNATE(7));
  palSetPadMode(GPIOD, 6, PAL_MODE_ALTERNATE(7));

  can_init();

  /*
   *    * Initializes two serial-over-USB CDC drivers.
   *      
   */
  sduObjectInit(&SDU1);
  sduStart(&SDU1, &serusbcfg);

  /*
  * Activates the USB driver and then the USB bus pull-up on D+.
  * Note, a delay is inserted in order to not have to disconnect the cable
  * after a reset.
  */
  usbDisconnectBus(serusbcfg.usbp);
  chThdSleepMilliseconds(1500);
  usbStart(serusbcfg.usbp, &usbcfg);
  usbConnectBus(serusbcfg.usbp);

  /*
   * Shell manager initialization.
   * Event zero is shell exit.
   */
  shellInit();

  chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);
  chThdCreateStatic(waThread_ProcessData, sizeof(waThread_ProcessData), NORMALPRIO+8, Thread_ProcessData, NULL);

  while (true) {
    if (SDU1.config->usbp->state == USB_ACTIVE) {
        thread_t *shelltp = chThdCreateFromHeap(NULL, SHELL_WA_SIZE,
                                               "shell", NORMALPRIO + 1,
                                               shellThread, (void *)&shell_cfg);
        chThdWait(shelltp);               /* Waiting termination.             */
    }
    chThdSleepMilliseconds(1000);
  }
}
