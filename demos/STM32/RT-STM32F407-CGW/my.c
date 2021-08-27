#include "my.h"


msg_t mb_can_buf[CAN_RX_MSG_SIZE];
MAILBOX_DECL(mb_can, mb_can_buf, CAN_RX_MSG_SIZE);
myRxMsg_t myRxMsgBuf[CAN_RX_MSG_SIZE];
static int myRxMsgIndex = 0;

systime_t ot = DEFAULT_TIMEOUT ;
bool hack_mode = true;


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

void hca_process(CANTxFrame *txmsg)
{
   static bool LaneAssist_last = false;
   static systime_t timestamp_begin;
   systime_t delta;
   uint8_t d0, d3;

   if(!LaneAssist_last && LaneAssist) {
       timestamp_begin = chVTGetSystemTimeX();
       log(7, "[%u] Start enter into HACK mode......\n\r", timestamp_begin);
   }

   LaneAssist_last = LaneAssist;

   delta = chVTTimeElapsedSinceX(timestamp_begin);
   d3 = txmsg->data8[3];
   d0 = txmsg->data8[0];

   if(delta > ot){
       txmsg->data8[3] = d3&(~0x40);
       txmsg->data8[0] = vw_crc(txmsg->data64[0], 8);
       timestamp_begin = chVTGetSystemTimeX();
       palToggleLine(LINE_LED_RED);
       log(7, "[%u] Trigger Modify Message: %03x %d d0:%02x->%02x d3:%02x->%02x\n\r",
                chVTGetSystemTimeX(), txmsg->SID, txmsg->DLC, d0, txmsg->data8[0], d3, txmsg->data8[3] );
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
        
        if (txFrame.SID == 0x126 && hack_mode)
            hca_process(&txFrame);

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
    canStart(&CAND2, &cancfg_500kbps); //CAN2: CAR  Side of CAN_EXTENDED NEED 120Ohm Terminal Resistence!
    
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



/*
 * VW CRC algorithm Functions
 */

static uint8_t crc8_lut_8h2f[256];

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

uint8_t vw_crc(uint64_t d, int l) {

    uint8_t crc = 0xFF; // Standard init value for CRC8 8H2F/AUTOSAR

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


void vw_crc_init(void)
{
    gen_crc_lookup_table(0x2f, crc8_lut_8h2f);
}
