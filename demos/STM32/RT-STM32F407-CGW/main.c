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
  uint8_t  len;
  int from;  // -1 is None
  int to;    // -1 is None
} tCAN_ROUTE_TABLE;

static thread_t *shelltp = NULL;

/*===========================================================================*/
/* Command line related.                                                     */
/*===========================================================================*/


#define SHELL_WA_SIZE   THD_WORKING_AREA_SIZE(2048)

static void cmd_cgw(BaseSequentialStream *chp, int argc, char *argv[]) {

    (void)argv;
    if (argc > 0) {
        chprintf(chp, "Usage: cgw\r\n");
        return;
    }
    
    chprintf(chp, "cmd_cgw\r\n");
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

struct can_instance {
  CANDriver     *canp;
  uint32_t      led;
  BaseSequentialStream *s;
};


static struct can_instance can1 = {&CAND1, GPIOA_LED_B, NULL};
static struct can_instance can2 = {&CAND2, GPIOA_LED_R, NULL};


void print_can_rx_msg( struct can_instance *cip, CANRxFrame *pMsg )
{
    BaseSequentialStream *s = cip->s;
    int len = pMsg->DLC;
    int i;

    if(cip->canp == &CAND1) {
            chprintf(s, "%01d",0);
    }
    else if(cip->canp == &CAND2){
            chprintf(s, "%01d",1);
    }
    else {
            chprintf(s, "ERROR %01d",-1);
            return;
    }

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
 * Receiver thread.
 */

static THD_WORKING_AREA(can_rx1_wa, 1024);
static THD_WORKING_AREA(can_rx2_wa, 1024);

static THD_FUNCTION(can_rx, p) {
  
  struct can_instance *cip = p;
  event_listener_t el;
  CANRxFrame rxmsg;

  (void)p;

  chRegSetThreadName("can_rx");
  chEvtRegister(&cip->canp->rxfull_event, &el, 0);

  while (true) {

    if (chEvtWaitAnyTimeout(ALL_EVENTS, TIME_MS2I(100)) == 0)
      continue;

    while (canReceive(cip->canp, CAN_ANY_MAILBOX,
                      &rxmsg, TIME_IMMEDIATE) == MSG_OK) {
      print_can_rx_msg(cip, &rxmsg);
      /* Process message.*/
      palTogglePad(GPIOA, cip->led);
    }
  }
  chEvtUnregister(&CAND1.rxfull_event, &el);
}

/*

 * Transmitter thread.

*/

static THD_WORKING_AREA(can_tx_wa, 256);

static THD_FUNCTION(can_tx, p) {
  CANTxFrame txmsg;
  int rt_count = 0;

  (void)p;
  
  chRegSetThreadName("transmitter");
  txmsg.IDE = CAN_IDE_EXT;
  txmsg.EID = 0x01234567;
  txmsg.RTR = CAN_RTR_DATA;
  txmsg.DLC = 8;
  txmsg.data32[0] = 0x55AA55AA;
  txmsg.data32[1] = 0x00FF00FF;

  while (true) {
    canTransmit(&CAND1, CAN_ANY_MAILBOX, &txmsg, TIME_MS2I(100));
    canTransmit(&CAND2, CAN_ANY_MAILBOX, &txmsg, TIME_MS2I(100));
    chThdSleepMilliseconds(500);
    rt_count = chSysGetRealtimeCounterX();
    txmsg.data32[1] = rt_count;

    //palTogglePad(GPIOA, GPIOA_LED_R);
  }

}



/*
 * This is a periodic thread that does absolutely nothing except flashing
 * a LED.
 */
static THD_WORKING_AREA(waThread1, 128);
static THD_FUNCTION(Thread1, arg) {

  (void)arg;
  chRegSetThreadName("blinker");
  while (true) {
    palSetPad(GPIOA, GPIOA_LED_G);       /* Green.  */
    chThdSleepMilliseconds(500);
    palClearPad(GPIOA, GPIOA_LED_G);     /* Green.  */
    chThdSleepMilliseconds(500);
  }
}

//
//static THD_WORKING_AREA(waThread2, 128);
//static THD_FUNCTION(Thread2, arg) {
//
//  (void)arg;
//  chRegSetThreadName("blinker");
//  while (true) {
//    palSetPad(GPIOA, GPIOA_LED_B);       /* Orange.  */
//    chThdSleepMilliseconds(200);
//    palClearPad(GPIOA, GPIOA_LED_B);     /* Orange.  */
//    chThdSleepMilliseconds(200);
//  }
//}
//



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
  chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);
  //chThdCreateStatic(waThread2, sizeof(waThread2), NORMALPRIO, Thread2, NULL);

  // Starting the transmitter and receiver threads
  chThdCreateStatic(can_rx1_wa, sizeof(can_rx1_wa), NORMALPRIO + 7,
                   can_rx, (void *)&can1);
  chThdCreateStatic(can_rx2_wa, sizeof(can_rx2_wa), NORMALPRIO + 7,
                   can_rx, (void *)&can2);
  chThdCreateStatic(can_tx_wa, sizeof(can_tx_wa), NORMALPRIO + 6,
                   can_tx, NULL);

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
