/*
    ChibiOS - Copyright (C) 2006..2018 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include <stdio.h>
#include <string.h>

#include "ch.h"
#include "hal.h"
#include "rt_test_root.h"
#include "oslib_test_root.h"
#include "shell.h"
#include "chprintf.h"


typedef struct { 
  bool IDE;
  bool RTR;
  uint32_t id;
  int len;   // -1 is Any length
  uint32_t count;
  uint32_t err_cnt;
} tCAN_FORWARD_TABLE;

static thread_t *shelltp = NULL;

struct can_instance {
  CANDriver     *canp;
  uint32_t      led;
  BaseSequentialStream *s;
};


//#define debug_printf(fmt, ...) chprintf(((BaseSequentialStream *)&SD2), fmt, ## __VA_ARGS__ )
#define debug_printf(fmt, ...) 
        
static struct can_instance can1 = {&CAND1, GPIOA_LED_B, NULL};
static struct can_instance can2 = {&CAND2, GPIOA_LED_G, NULL};

tCAN_FORWARD_TABLE cgw_b0tob1_tbl[] = {
    //from Bus0 to Bus1
    {1,0,0x16F1F018,-1,0, 0},
};

tCAN_FORWARD_TABLE cgw_b1tob0_tbl[] = {
    //from Bus1 to Bus0
    {1,0,0x16F1F018,-1,0,0},
    {1,0,0x12F8BFA7,-1,0,0},
    {1,0,0x12F8BE9F,-1,0,0},
};

/*
 * Receiver thread.
 */

static THD_WORKING_AREA(can_b0tob1_wa, 1024);

static THD_FUNCTION(can_b0tob1, arg) {
  
  (void)arg;

  //struct can_instance *cip = p;
  int i,tbl_len;
  event_listener_t el;
  CANRxFrame rxmsg;
  CANTxFrame txmsg;

  chRegSetThreadName("can_b0tob1");
  chEvtRegister(&(can1.canp->rxfull_event), &el, 0);

  tbl_len = sizeof(cgw_b0tob1_tbl)/sizeof(tCAN_FORWARD_TABLE);

  while (true) {

    if (chEvtWaitAnyTimeout(ALL_EVENTS, TIME_MS2I(100)) == 0)
      continue;

    while (canReceive(can1.canp, CAN_ANY_MAILBOX,
                      &rxmsg, TIME_IMMEDIATE) == MSG_OK) {
      /* Process message.*/
      palTogglePad(GPIOA, can1.led);

      for(i=0; i<tbl_len; i++){
          if( (cgw_b0tob1_tbl[i].IDE == rxmsg.IDE) 
           && (cgw_b0tob1_tbl[i].RTR == rxmsg.RTR) 
           && ( ( (cgw_b0tob1_tbl[i].id == rxmsg.EID)&&(rxmsg.IDE==1) )
              || ( (cgw_b0tob1_tbl[i].id == rxmsg.SID)&&(rxmsg.IDE==0) ) )
           && ( (cgw_b0tob1_tbl[i].len == rxmsg.DLC)||(cgw_b0tob1_tbl[i].len == -1) )
           ){
              cgw_b0tob1_tbl[i].count++;
              txmsg.DLC = rxmsg.DLC;
              txmsg.RTR = rxmsg.RTR;
              txmsg.IDE = rxmsg.IDE;
              txmsg.data32[0] = rxmsg.data32[0];
              txmsg.data32[1] = rxmsg.data32[1];
              txmsg.EID = rxmsg.EID;

              if( canTransmit(can2.canp, CAN_ANY_MAILBOX, &txmsg, TIME_IMMEDIATE) != MSG_OK ){
                palTogglePad(GPIOA, GPIOA_LED_R);
                cgw_b0tob1_tbl[i].err_cnt++;
                debug_printf("b0->b1:%d\r\n",cgw_b0tob1_tbl[i].err_cnt++);
              }
          }
      } 
      
    }
  }
  chEvtUnregister(&CAND1.rxfull_event, &el);
}

static THD_WORKING_AREA(can_b1tob0_wa, 1024);

static THD_FUNCTION(can_b1tob0, arg) {
  
  (void)arg;

  //struct can_instance *cip = p;
  int i,tbl_len;
  event_listener_t el;
  CANRxFrame rxmsg;
  CANTxFrame txmsg;

  chRegSetThreadName("can_b1tob0");
  chEvtRegister(&(can2.canp->rxfull_event), &el, 0);

  tbl_len = sizeof(cgw_b1tob0_tbl)/sizeof(tCAN_FORWARD_TABLE);

  while (true) {

    if (chEvtWaitAnyTimeout(ALL_EVENTS, TIME_MS2I(100)) == 0)
      continue;

    while (canReceive(can2.canp, CAN_ANY_MAILBOX,
                      &rxmsg, TIME_IMMEDIATE) == MSG_OK) {
      /* Process message.*/
      palTogglePad(GPIOA, can2.led);

      for(i=0; i<tbl_len; i++){
          if( (cgw_b1tob0_tbl[i].IDE == rxmsg.IDE) 
           && (cgw_b1tob0_tbl[i].RTR == rxmsg.RTR) 
           && ( ( (cgw_b1tob0_tbl[i].id == rxmsg.EID)&&(rxmsg.IDE==1) )
             || ( (cgw_b1tob0_tbl[i].id == rxmsg.SID)&&(rxmsg.IDE==0) ) )
           && ( (cgw_b1tob0_tbl[i].len == rxmsg.DLC)||(cgw_b1tob0_tbl[i].len == -1) )
           ){
              cgw_b1tob0_tbl[i].count++;
              txmsg.DLC = rxmsg.DLC;
              txmsg.RTR = rxmsg.RTR;
              txmsg.IDE = rxmsg.IDE;
              txmsg.data32[0] = rxmsg.data32[0];
              txmsg.data32[1] = rxmsg.data32[1];
              txmsg.EID = rxmsg.EID;

              if( canTransmit(can1.canp, CAN_ANY_MAILBOX, &txmsg, TIME_IMMEDIATE) != MSG_OK ){
                palTogglePad(GPIOA, GPIOA_LED_R);
                cgw_b1tob0_tbl[i].err_cnt++;
                debug_printf("b1->b0:%d\r\n",cgw_b1tob0_tbl[i].err_cnt++);
              }
          }
      } 
      
    }
  }
  chEvtUnregister(&CAND2.rxfull_event, &el);
}


/*===========================================================================*/
/* Command line related.                                                     */
/*===========================================================================*/


#define SHELL_WA_SIZE   THD_WORKING_AREA_SIZE(2048)

static void cmd_cgw(BaseSequentialStream *chp, int argc, char *argv[]) {

    (void)argv;
    
    int i, len;

    if (argc > 0) {
        chprintf(chp, "Usage: cgw\r\n");
        return;
    }

    chprintf(chp, "cgw:\r\n");

    chprintf(chp, "forward from b0 to b1 ...\r\n");
    len = sizeof(cgw_b0tob1_tbl)/sizeof(tCAN_FORWARD_TABLE);
    for(i=0; i<len; i++){
        chprintf(chp, "[%d] IDE=%d RTR=%d DLC=%d %08x %010d(%010d)\r\n"
                ,i
                ,cgw_b0tob1_tbl[i].IDE
                ,cgw_b0tob1_tbl[i].RTR
                ,cgw_b0tob1_tbl[i].len
                ,cgw_b0tob1_tbl[i].id
                ,cgw_b0tob1_tbl[i].count
                ,cgw_b0tob1_tbl[i].err_cnt
                );
    }
    
    chprintf(chp, "\r\n");
    chprintf(chp, "forward from b0 to b1 ...\r\n");
    len = sizeof(cgw_b1tob0_tbl)/sizeof(tCAN_FORWARD_TABLE);
    for(i=0; i<len; i++){
        chprintf(chp, "[%d] IDE=%d RTR=%d DLC=%d %08x %010d(%010d)\r\n" 
                ,i
                ,cgw_b1tob0_tbl[i].IDE
                ,cgw_b1tob0_tbl[i].RTR
                ,cgw_b1tob0_tbl[i].len
                ,cgw_b1tob0_tbl[i].id
                ,cgw_b1tob0_tbl[i].count
                ,cgw_b1tob0_tbl[i].err_cnt
                );
    }
    chprintf(chp, "\r\n");
    

    return;
                    
}

static const ShellCommand commands[] = {
      {"cgw", cmd_cgw},
      {NULL, NULL}
      
};


static const ShellConfig shell_cfg1 = {
      (BaseSequentialStream *)&SD2,
      commands
};

/*

 * Internal loopback mode, 500KBaud, automatic wakeup, automatic recover

 * from abort mode.

 * See section 22.7.7 on the STM32 reference manual.

 */

/*
 *
 * PCK1 : 42Mhz
 *
 * SJW: 0-1
 * TS1: 0-15
 * TS2: 0-8
 *
 * 
 * 500Kbps :
 *  - PCK1=42MHz
 *  - BRP =(6+1) = 7
 *  - SJW+TS1+TS2 =(0+1) + (8+1) + (1+1) = 12
 *
 *  We get:
 *  1/Tq = PCK1/BRP = 42/7 = 6MHz 
 *  1bit timing = 12Tq , CANBaud=6Mhz/12=0.5Mhz=500Kbps
 *  
 *  - PCK1=42MHz
 *  - BRP =(13+1) = 14 
 *  - SJW+TS1+TS2 =(0+1) + (8+1) + (1+1) = 12
 *
 *  1/Tq = PCK1/BRP = 42/14 = 3MHz 
 *  1bit timing = 12Tq , CANBaud=3Mhz/12=0.25Mhz=250Kbps
 *
 *  - PCK1=42MHz
 *  - BRP =(13+1) = 14 
 *  - SJW+TS1+TS2 =(0+1) + (14+1) + (7+1) = 24 
 *
 *  1/Tq = PCK1/BRP = 42/14 = 3MHz 
 *  1bit timing = 24Tq , CANBaud=3Mhz/24=0.125Mhz=125Kbps
 */

static const CANConfig cancfg_500kbps = {

  CAN_MCR_ABOM | CAN_MCR_AWUM | CAN_MCR_TXFP,
  CAN_BTR_SJW(0) | CAN_BTR_TS2(1) |
  CAN_BTR_TS1(8) | CAN_BTR_BRP(6)

};

static const CANConfig cancfg_250kbps = {

  CAN_MCR_ABOM | CAN_MCR_AWUM | CAN_MCR_TXFP,
  CAN_BTR_SJW(0) | CAN_BTR_TS2(1) |
  CAN_BTR_TS1(8) | CAN_BTR_BRP(13)

};

static const CANConfig cancfg_125kbps = {

  CAN_MCR_ABOM | CAN_MCR_AWUM | CAN_MCR_TXFP,
  CAN_BTR_SJW(0) | CAN_BTR_TS2(7) |
  CAN_BTR_TS1(14) | CAN_BTR_BRP(13)

};


void print_can_rx_msg( BaseSequentialStream *s, int ch, CANRxFrame *pMsg )
{
    int len = pMsg->DLC;
    int i;

    chprintf(s, "%01d", ch);

    if(pMsg->RTR) {
      chprintf(s, " R");
    }

    chprintf(s,  " %d", len);

    if(pMsg->IDE) {
      chprintf(s, " E %08X", pMsg->EID&0x3fffffff);
    }
    else {
      chprintf(s, " S %03X", pMsg->SID&0x7ff);
    }

    for(i=0; i<len; i++){
        chprintf(s, " %02X", pMsg->data8[i]);
    }

    chprintf(s, "\n\r");

}


/*
 * Application entry point.
 */
int main(void) {

  // static const evhandler_t evhndl[] = {
  //    ShellHandler
  // };
  // event_listener_t el0;


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
   *    * Shell manager initialization.
   *       */
  shellInit();


  /*
   * Activates the serial driver 2 using the driver default configuration.
   * PD5(TX) and PD6(RX) are routed to USART2.
   */
  sdStart(&SD2, NULL);
  palSetPadMode(GPIOD, 5, PAL_MODE_ALTERNATE(7));
  palSetPadMode(GPIOD, 6, PAL_MODE_ALTERNATE(7));

  /*

   * Activates the CAN drivers 1 and 2.

  */

  canStart(&CAND1, &cancfg_500kbps); //Bus0: OP CAR-CAN
  canStart(&CAND2, &cancfg_125kbps); //Bus1: B-CAN
  palSetPadMode(GPIOB, 8, PAL_MODE_ALTERNATE(9));
  palSetPadMode(GPIOB, 9, PAL_MODE_ALTERNATE(9));
  palSetPadMode(GPIOB, 13, PAL_MODE_ALTERNATE(9));
  palSetPadMode(GPIOB, 5, PAL_MODE_ALTERNATE(9));
  

  palSetPadMode(GPIOC, 7, PAL_MODE_OUTPUT_PUSHPULL);
  palSetPadMode(GPIOC, 9, PAL_MODE_OUTPUT_PUSHPULL);

  palClearPad(GPIOC, 7); 
  palClearPad(GPIOC, 9);
  can1.s = (BaseSequentialStream *) &SD2;
  can2.s = (BaseSequentialStream *) &SD2;

  /*
   * Creates the example thread.
   */
  // Starting the transmitter and receiver threads
  chThdCreateStatic(can_b0tob1_wa, sizeof(can_b0tob1_wa), NORMALPRIO + 7,
                   can_b0tob1, (void *)NULL);
  chThdCreateStatic(can_b1tob0_wa, sizeof(can_b1tob0_wa), NORMALPRIO + 7,
                   can_b1tob0, (void *)NULL);

  //chEvtRegister(&shell_terminated, &el0, 0);

  while (TRUE) {  
    thread_t *shelltp = chThdCreateFromHeap(NULL, SHELL_WA_SIZE,
                                            "shell", NORMALPRIO + 1,
                                            shellThread, (void *)&shell_cfg1);
    chThdWait(shelltp);               /* Waiting termination.             */
    chThdSleepMilliseconds(1000);
    //chprintf((BaseSequentialStream *) &SD2, "hello\n\r");
  } 
}
