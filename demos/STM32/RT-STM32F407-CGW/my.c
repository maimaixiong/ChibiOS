#include "my.h"


msg_t mb_can_buf[CAN_RX_MSG_SIZE];
MAILBOX_DECL(mb_can, mb_can_buf, CAN_RX_MSG_SIZE);
myRxMsg_t myRxMsgBuf[CAN_RX_MSG_SIZE];
static int myRxMsgIndex = 0;


int putMailMessage(int can_bus, CANRxFrame* pf)
{
    myRxMsg_t *myMsg;
    myMsg = &myRxMsgBuf[myRxMsgIndex];
    myMsg->can_bus = can_bus;
    myMsg->timestamp = chVTGetSystemTimeX();
    memcpy( &(myMsg->fr), pf, sizeof(CANRxFrame) );
    chMBPostI(&mb_can, (msg_t) myRxMsgIndex);
    myRxMsgIndex++;
    if (myRxMsgIndex>=CAN_RX_MSG_SIZE) myRxMsgIndex = 0;
    return myRxMsgIndex;
}

int getMailMessage(myRxMsg_t *pMsg)
{
    msg_t msg, msgsts;
    
    msgsts = chMBFetchTimeout(&mb_can, &msg, TIME_INFINITE);

    if( msgsts < MSG_OK ) {
        log(7, "getMailMessge() is fail!\n\r");
        return -1;
    }

    memcpy(pMsg, &myRxMsgBuf[msg], sizeof(myRxMsg_t));

    return 0;
}

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

void candump(myRxMsg_t *pMsg)
{
    int level = LOG_LEVEL_INFO;
    int i;

    CANRxFrame* cf;
    cf = &(pMsg->fr);

    log(level, "%10ld %d %8x%c%c %02u ", 
            pMsg->timestamp,
            pMsg->can_bus,
            cf->EID,
            (cf->IDE)   ? 'x':' ',
            (cf->RTR)   ? 'R':' ',
            cf->DLC            
            );
    for(i=0; i<cf->DLC; i++) log(level, "%02x ", cf->data8[i]);
    log(level, "\n\r");
}

void canframe_copy( CANTxFrame *tx, CANRxFrame *rx )
{
   tx->IDE = rx->IDE;
   if (rx->IDE)
        tx->EID = rx->EID;
   else
        tx->SID = rx->SID;

   tx->DLC = rx->DLC;
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
        palToggleLine(LINE_LED_BLUE);
        canframe_copy(&txFrame, &rxFrame);
        putMailMessage(1, &rxFrame);
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
        putMailMessage(2, &rxFrame);
        if (canTryTransmitI(&CAND1, CAN_ANY_MAILBOX, &txFrame)) {
            err_count++;
            log(7, "CAN.SEND.ERR: CAN%d->%d ID=%x X=%d R=%d L=%d (err_count=%d)\n\r", 2, 1, rxFrame.EID, rxFrame.EID, rxFrame.RTR, rxFrame.DLC, err_count);
        }
    } 
}


void can_init(void)
{
    chMBObjectInit(&mb_can, mb_can_buf, CAN_RX_MSG_SIZE );

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
