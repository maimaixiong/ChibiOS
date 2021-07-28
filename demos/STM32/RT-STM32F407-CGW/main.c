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

#include "dbc/vw.h"


static thread_t *shelltp = NULL;

struct can_instance {
  CANDriver     *canp;
  uint32_t      led;
  BaseSequentialStream *s;
};


struct can_msg_info {
    uint32_t id;
    uint32_t timestamp;
    uint32_t count;
    uint32_t t_max;
    uint32_t t_min;
    uint32_t t_avg;
};

typedef struct can_msg_info can_msg_info_t;


#define debug_printf(fmt, ...) chprintf(((BaseSequentialStream *)&SD2), fmt, ## __VA_ARGS__ )
//#define debug_printf(fmt, ...) 
#define printf(fmt, ...) chprintf(((BaseSequentialStream *)&SD2), fmt, ## __VA_ARGS__ )
        
static struct can_instance can1 = {&CAND1, GPIOA_LED_B, NULL};
static struct can_instance can2 = {&CAND2, GPIOA_LED_G, NULL};

static can_obj_vw_h_t vw_obj_from_bus0;
static can_obj_vw_h_t vw_obj_from_bus1;
static bool hca_err, acc_enable, stop_still; 


#define MSG_MAX_CNT 200
#define FLT_AVG_NUM 10

can_msg_info_t bus_info_0[MSG_MAX_CNT];
can_msg_info_t bus_info_1[MSG_MAX_CNT];

void can_msg_info_add(can_msg_info_t *p, uint32_t id, uint32_t timestamp)
{
    int i;
    uint32_t delta_timestamp;

    for(i=0; i<MSG_MAX_CNT; i++){

        if( p[i].id == 0 ) {
            p[i].id = id;
            p[i].timestamp = timestamp;
            p[i].t_max = 0;
            p[i].t_min = 0xffffffff;
            p[i].t_avg = 0;
            p[i].count = 1;
            break;
        }

        if( p[i].id == id ) {
            delta_timestamp = chVTTimeElapsedSinceX(p[i].timestamp);
            p[i].timestamp = timestamp;
            p[i].count ++;
            p[i].t_min = (delta_timestamp <= p[i].t_min)? delta_timestamp: p[i].t_min;
            p[i].t_max = (delta_timestamp >= p[i].t_max)? delta_timestamp: p[i].t_max;

            p[i].t_avg = ( p[i].t_avg * (FLT_AVG_NUM-1) + delta_timestamp )/ (FLT_AVG_NUM);

            break;
        }
    }
}

void bus_msg_info_disp(BaseSequentialStream *chp, can_msg_info_t *p)
{
    int i;
    
    chprintf(chp, "-----------------------------------------------\r\n");

    for(i=0; i<MSG_MAX_CNT; i++){
        if( p[i].id != 0 ) {
            chprintf(chp, "%d\t0x%x(%u)\t%u\t%u\t(%u:%u:%u)\r\n", 
                    i, p[i].id, p[i].id, p[i].timestamp, p[i].count, p[i].t_min ,p[i].t_avg, p[i].t_max);
        }
    }

    chprintf(chp, "-----------------------------------------------\r\n");
}

static uint64_t u64_from_can_msg(const uint8_t m[8]) {
	return ((uint64_t)m[7] << 56) | ((uint64_t)m[6] << 48) | ((uint64_t)m[5] << 40) | ((uint64_t)m[4] << 32)
		| ((uint64_t)m[3] << 24) | ((uint64_t)m[2] << 16) | ((uint64_t)m[1] << 8) | ((uint64_t)m[0] << 0);
}

static void u64_to_can_msg(const uint64_t u, uint8_t m[8]) {
	m[7] = u >> 56;
	m[6] = u >> 48;
	m[5] = u >> 40;
	m[4] = u >> 32;
	m[3] = u >> 24;
	m[2] = u >> 16;
	m[1] = u >>  8;
	m[0] = u >>  0;
}

void gen_crc_lookup_table(uint8_t poly, uint8_t crc_lut[]) {
  uint8_t crc;
  int i, j;

   for (i = 0; i < 256; i++) {
    crc = i;
    for (j = 0; j < 8; j++) {
      if ((crc & 0x80) != 0)
        crc = (uint8_t)((crc << 1) ^ poly);
      else
        crc <<= 1;
    }
    crc_lut[i] = crc;
  }
}

// Static lookup table for fast computation of CRC8 poly 0x2F, aka 8H2F/AUTOSAR
uint8_t crc8_lut_8h2f[256] ;

// 读取小端数
uint64_t read_u64_le(const uint8_t* v) {
  return ((uint64_t)v[0]
          | ((uint64_t)v[1] << 8)
          | ((uint64_t)v[2] << 16)
          | ((uint64_t)v[3] << 24)
          | ((uint64_t)v[4] << 32)
          | ((uint64_t)v[5] << 40)
          | ((uint64_t)v[6] << 48)
          | ((uint64_t)v[7] << 56));
}

unsigned int vw_crc(unsigned int address, uint8_t *data, int l) {

  uint64_t d;
  uint8_t crc = 0xFF; // Standard init value for CRC8 8H2F/AUTOSAR

  d = read_u64_le(data);

  // Volkswagen uses standard CRC8 8H2F/AUTOSAR, but they compute it with
  // a magic variable padding byte tacked onto the end of the payload.
  // https://www.autosar.org/fileadmin/user_upload/standards/classic/4-3/AUTOSAR_SWS_CRCLibrary.pdf


  // CRC the payload first, skipping over the first byte where the CRC lives.
  for (int i = 1; i < l; i++) {
    crc ^= (d >> (i*8)) & 0xFF;
    crc = crc8_lut_8h2f[crc];
  }

  // Look up and apply the magic final CRC padding byte, which permutes by CAN
  // address, and additionally (for SOME addresses) by the message counter.
  uint8_t counter = ((d >> 8) & 0xFF) & 0x0F;
  crc ^= (uint8_t[]){0xDA,0xDA,0xDA,0xDA,0xDA,0xDA,0xDA,0xDA,0xDA,0xDA,0xDA,0xDA,0xDA,0xDA,0xDA,0xDA}[counter];
  crc = crc8_lut_8h2f[crc];

  return crc ^ 0xFF; // Return after standard final XOR for CRC8 8H2F/AUTOSAR
}


uint8_t hca_stat=0;
uint8_t tsk_stat=0;
double wheelSpeeds_fl=0;
double wheelSpeeds_fr=0;
double wheelSpeeds_rl=0;
double wheelSpeeds_rr=0;
double vEgoRaw=0;

/*
 * Receiver thread.
 */

static THD_WORKING_AREA(can_b0tob1_wa, 1024);

/*
 * receive can message from bus0(J533) 
 * unpack can message and set acc_enable,stopstill,hca_err
 * forward can message to bus1(car extended can)
 */

static THD_FUNCTION(can_b0tob1, arg) {
  
  (void)arg;
  bool forward=true;

  //struct can_instance *cip = p;
  event_listener_t el;
  CANRxFrame rxmsg;
  CANTxFrame txmsg;
  uint64_t data;
  uint8_t  dlc;
  int i;
  uint8_t chksum;

  chRegSetThreadName("can_b0tob1");
  chEvtRegister(&(can1.canp->rxfull_event), &el, 0);

  acc_enable = false;
  hca_err = true;
  stop_still = true;

  while (true) {

    if (chEvtWaitAnyTimeout(ALL_EVENTS, TIME_MS2I(100)) == 0)
      continue;

    while (canReceive(can1.canp, CAN_ANY_MAILBOX,
                      &rxmsg, TIME_IMMEDIATE) == MSG_OK) {
      /* Process message.*/
      palTogglePad(GPIOA, can1.led);
      data = u64_from_can_msg(rxmsg.data8);
      dlc  = rxmsg.DLC;
      forward = true;

      can_msg_info_add(bus_info_0, rxmsg.EID,  chVTGetSystemTimeX());

      if (unpack_message(&vw_obj_from_bus0, rxmsg.SID, data , dlc, chVTGetSystemTimeX()) < 0) {
	    // Error Condition; something went wrong
	    //return -1;
        //debug_printf("[ERROR]b02b1 unpack_message(%x,%d)!\r\n", rxmsg.SID, dlc);
      }
      else {
        chksum = vw_crc(0x126, rxmsg.data8, 8);
        //chprintf(&SD2, "[%s] data0:%02x, chksum must be :%02x\r\n",(chksum==rxmsg.data8[0]?"O":"X"), rxmsg.data8[0], chksum);
        //print_message(&vw_obj_from_bus0, rxmsg.SID, &SD2);

        hca_stat = vw_obj_from_bus0.can_0x09f_LH_EPS_03.EPS_HCA_Status;
        hca_err = ( hca_stat==0 || hca_stat==1 || hca_stat==2 || hca_stat==4 );

        tsk_stat = vw_obj_from_bus0.can_0x120_TSK_06.TSK_Status;
        acc_enable = ( tsk_stat==3 || tsk_stat==4 || tsk_stat==5 );

        decode_can_0x0b2_ESP_VL_Radgeschw_02(&vw_obj_from_bus0, &wheelSpeeds_fl);
        decode_can_0x0b2_ESP_VR_Radgeschw_02(&vw_obj_from_bus0, &wheelSpeeds_fr);
        decode_can_0x0b2_ESP_HL_Radgeschw_02(&vw_obj_from_bus0, &wheelSpeeds_rl);
        decode_can_0x0b2_ESP_HR_Radgeschw_02(&vw_obj_from_bus0, &wheelSpeeds_rr);

        vEgoRaw = (wheelSpeeds_fl + wheelSpeeds_fr + wheelSpeeds_rl + wheelSpeeds_rr)/4;
        stop_still = (vEgoRaw<1);

        //chprintf(&SD2, "EgoSpeed %u:[FL:%u,FR:%u,RF:%u,RR:%u],HCA_STAT:%u, TSK_STAT:%u\r\n",
        //    vEgoRaw, wheelSpeeds_fl, wheelSpeeds_fr, wheelSpeeds_rl, wheelSpeeds_rr, hca_stat, tsk_stat);

      }

      if(forward) 
      {
        txmsg.DLC = rxmsg.DLC;
        txmsg.RTR = rxmsg.RTR;
        txmsg.IDE = rxmsg.IDE;
        txmsg.data32[0] = rxmsg.data32[0];
        txmsg.data32[1] = rxmsg.data32[1];
        txmsg.EID = rxmsg.EID;

        if( canTransmit(can2.canp, CAN_ANY_MAILBOX, &txmsg, TIME_IMMEDIATE) != MSG_OK ){
          //palTogglePad(GPIOA, GPIOA_LED_R);
          debug_printf("[ERROR] b02b1 canTransmit(%x,%d)!\r\n", txmsg.IDE, txmsg.DLC);
        }
      }

    }
  }
  chEvtUnregister(&CAND1.rxfull_event, &el);
}

/*
 * receive can message from bus1(car extended can) 
 * unpack can message and set acc_enable,stopstill,hca_err
 * forward can message to bus1(car extended can)
 */

static THD_WORKING_AREA(can_b1tob0_wa, 1024);

static THD_FUNCTION(can_b1tob0, arg) {
  
  (void)arg;

  uint32_t time_stamp = 0;
  uint32_t last_timestamp = 0;
  uint8_t checksum =0;

  bool assist_req = false;

  //struct can_instance *cip = p;
  event_listener_t el;
  CANRxFrame rxmsg;
  CANTxFrame txmsg;

  chRegSetThreadName("can_b1tob0");
  chEvtRegister(&(can2.canp->rxfull_event), &el, 0);


  while (true) {

    if (chEvtWaitAnyTimeout(ALL_EVENTS, TIME_MS2I(100)) == 0)
      continue;

    while (canReceive(can2.canp, CAN_ANY_MAILBOX,
                      &rxmsg, TIME_IMMEDIATE) == MSG_OK) {
      /* Process message.*/
      palTogglePad(GPIOA, can2.led);

      can_msg_info_add(bus_info_1, rxmsg.EID,  chVTGetSystemTimeX());

      //if (unpack_message(&vw_obj_from_bus1, rxmsg.SID, data , dlc, 0) < 0) {
      //}
      //else {
      //}

      if( !hca_err && acc_enable && !stop_still ){
        //chprintf(&SD2, "Enter into Hijack......\r\n");
        palClearPad(GPIOA, GPIOA_LED_R); //Turn ON LED

        if(rxmsg.SID == 0x126 ) 
        {
          //forward message HCA
          time_stamp = chVTGetSystemTimeX();          
          //delta_timestamp = chVTTimeElapsedSinceX(last_timestamp);

          txmsg.DLC = rxmsg.DLC;
          txmsg.RTR = rxmsg.RTR;
          txmsg.IDE = rxmsg.IDE;
          txmsg.EID = rxmsg.EID;
          
          txmsg.data32[0] = rxmsg.data32[0];
          txmsg.data32[1] = rxmsg.data32[1];

          txmsg.data8[3] = (rxmsg.data8[3]&0xBF) | ((assist_req)?0x40:0x00);
          
          checksum = vw_crc(0x126, rxmsg.data8, 8);
          txmsg.data8[0] = checksum;
          
          //chprintf(&SD2, "Hijacking ...\r\n");
        }

      }
      else {
          palSetPad(GPIOA, GPIOA_LED_R);  //Turn Off LED
          //chprintf(&SD2, "Exit from Hijack......\r\n");
          //forward message except HCA
          txmsg.DLC = rxmsg.DLC;
          txmsg.RTR = rxmsg.RTR;
          txmsg.IDE = rxmsg.IDE;
          txmsg.data32[0] = rxmsg.data32[0];
          txmsg.data32[1] = rxmsg.data32[1];
          txmsg.EID = rxmsg.EID;

      } 

      if( canTransmit(can1.canp, CAN_ANY_MAILBOX, &txmsg, TIME_IMMEDIATE) != MSG_OK ){
        //palTogglePad(GPIOA, GPIOA_LED_R);
      }
      
    }
  }
  chEvtUnregister(&CAND2.rxfull_event, &el);
}


/*===========================================================================*/
/* Command line related.                                                     */
/*===========================================================================*/


#define SHELL_WA_SIZE   THD_WORKING_AREA_SIZE(2048)

static void cmd_vwinfo(BaseSequentialStream *chp, int argc, char *argv[]) {

    (void)argv;
    
    chprintf(chp, "\r\n-------------------------------------------------------------\r\n");
    chprintf(chp, "EgoSpeed %d:[FL:%u,FR:%d,RF:%d,RR:%d],HCA_STAT:%d, TSK_STAT:%d, HCA_ERROR:%d ACC_Enbale:%d STOP:%d",
            (int)vEgoRaw, (int)wheelSpeeds_fl, (int)wheelSpeeds_fr, (int)wheelSpeeds_rl, (int)wheelSpeeds_rr, hca_stat, tsk_stat, hca_err, acc_enable, stop_still);
    chprintf(chp, "\r\n");
    return;
    
}

static void cmd_businfo(BaseSequentialStream *chp, int argc, char *argv[]) {

    (void)argv;
    int bus_num;
    can_msg_info_t *bus;
    
    if (argc != 1) {
        chprintf(chp, "Usage: businfo bus_num[0/1/-1]\r\n");
        return;
    }

    if (!strcmp(argv[0], "-1")) {
        bus_num = -1;
        memset( bus_info_0, 0, sizeof(bus_info_0) );
        memset( bus_info_1, 0, sizeof(bus_info_1) );
        chprintf(chp, "clear bus0/1 info.\r\n");
        return;
    }
    else if (!strcmp(argv[0], "0")) {
        bus_num = 0;
        bus = bus_info_0;
    }
    else {
        bus_num = 1;
        bus = bus_info_1;
    }

    chprintf(chp, "businfo: %d\r\n", bus_num);
    bus_msg_info_disp(chp, bus);

    chprintf(chp, "\r\n");
    return;
                    
}

static const ShellCommand commands[] = {
      {"businfo", cmd_businfo},
      {"vwinfo",  cmd_vwinfo},
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

  canStart(&CAND1, &cancfg_500kbps); //Bus0: J533
  canStart(&CAND2, &cancfg_500kbps); //Bus1: CAR
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

  gen_crc_lookup_table(0x2F, crc8_lut_8h2f);
  memset( bus_info_0, 0, sizeof(bus_info_0) );
  memset( bus_info_1, 0, sizeof(bus_info_1) );

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
