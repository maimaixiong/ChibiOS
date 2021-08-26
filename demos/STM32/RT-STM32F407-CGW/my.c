#include "my.h"


/*===========================================================================*/
/* Module local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Module exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Module local types.                                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Module local variables.                                                   */
/*===========================================================================*/

static const CANConfig cancfg_500kbps = {
      CAN_MCR_ABOM | CAN_MCR_AWUM | CAN_MCR_TXFP,
      CAN_BTR_SJW(0) | CAN_BTR_TS2(1) |
      CAN_BTR_TS1(8) | CAN_BTR_BRP(6)
};

#if 0
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
#endif


void canframe_copy( CANTxFrame *tx, CANRxFrame *rx )
{
   tx->EID = rx->EID;
   tx->DLC = rx->DLC;
   tx->IDE = rx->IDE;
   tx->RTR = rx->RTR;
   tx->data64[0] = rx->data64[0];
}

void can1_gw2car(CANDriver *canp, uint32_t flags)
{   
    static CANRxFrame rxFrame;
    static CANTxFrame txFrame;
    static int err_count=0;

    (void)flags;
    (void)canp;
    
    if(!canTryReceiveI(&CAND1,CAN_ANY_MAILBOX,&rxFrame)){
        canframe_copy(&txFrame, &rxFrame);
        if (canTryTransmitI(&CAND2, CAN_ANY_MAILBOX, &txFrame)) {
            err_count++;
            log(7, "CAN.SEND.ERR: CAN%d->%d ID=%x X=%d R=%d L=%d (err_count=%d)\n\r", 1, 2, rxFrame.EID, rxFrame.EID, rxFrame.RTR, rxFrame.DLC, err_count);
        }
    } 
}

void can2_car2gw(CANDriver *canp, uint32_t flags)
{   
    static CANRxFrame rxFrame;
    static CANTxFrame txFrame;
    static int err_count=0;

    (void)flags;
    (void)canp;
    
    if(!canTryReceiveI(&CAND2,CAN_ANY_MAILBOX,&rxFrame)){
        canframe_copy(&txFrame, &rxFrame);
        if (canTryTransmitI(&CAND1, CAN_ANY_MAILBOX, &txFrame)) {
            err_count++;
            log(7, "CAN.SEND.ERR: CAN%d->%d ID=%x X=%d R=%d L=%d (err_count=%d)\n\r", 2, 1, rxFrame.EID, rxFrame.EID, rxFrame.RTR, rxFrame.DLC, err_count);
        }
    } 
}


void can_init(void)
{
    CAND1.rxfull_cb = can1_gw2car;
    CAND2.rxfull_cb = can2_car2gw;
    
    canStart(&CAND1, &cancfg_500kbps); //CAN1: J533 Side of CAN_EXTENDED
    canStart(&CAND2, &cancfg_500kbps); //CAN2: CAR  Side of CAN_EXTENDED
    
    palSetPadMode(GPIOB, 8, PAL_MODE_ALTERNATE(9));
    palSetPadMode(GPIOB, 9, PAL_MODE_ALTERNATE(9));
    palSetPadMode(GPIOB, 13, PAL_MODE_ALTERNATE(9));
    palSetPadMode(GPIOB, 5, PAL_MODE_ALTERNATE(9));
                    
    // CAN Tranceiver Enable
    palSetPadMode(GPIOC, 7, PAL_MODE_OUTPUT_PUSHPULL);
    palSetPadMode(GPIOC, 9, PAL_MODE_OUTPUT_PUSHPULL);
    palClearPad(GPIOC, 7); 
    palClearPad(GPIOC, 9);
    
}
