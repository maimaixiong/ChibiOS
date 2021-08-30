#include "my.h"

#define CANID_DELIM '#'
#define CC_DLC_DELIM '_'
#define DATA_SEPERATOR '.'
#define CAN_MAX_DLEN 8 
#define CANFD_MAX_DLEN 64
#define CAN_MAX_RAW_DLC 8

#define CAN_MTU 1
#define CANFD_MTU 2


int parse_canframe(char *cs,  CANTxFrame *cf) {
	/* documentation see lib.h */

	int i, idx, dlen, len;
	int maxdlen = CAN_MAX_DLEN;
	int ret = CAN_MTU;
	unsigned char tmp;

	len = strlen(cs);
	//printf("'%s' len %d\n", cs, len);

	memset(cf, 0, sizeof(*cf)); /* init CAN FD frame, e.g. LEN = 0 */

	if (len < 4)
		return 0;

	if (cs[3] == CANID_DELIM) { /* 3 digits */

		idx = 4;
		for (i=0; i<3; i++){
			if ((tmp = asc2nibble(cs[i])) > 0x0F)
				return 0;
			cf->EID |= (tmp << (2-i)*4);
		}
			cf->IDE = 0;   /* then it is an extended frame */

	} else if (cs[8] == CANID_DELIM) { /* 8 digits */

		idx = 9;
		for (i=0; i<8; i++){
			if ((tmp = asc2nibble(cs[i])) > 0x0F)
				return 0;
			cf->EID |= (tmp << (7-i)*4);
		}
			cf->IDE = 1;   /* then it is an extended frame */

	} else
		return 0;

	if((cs[idx] == 'R') || (cs[idx] == 'r')){ /* RTR frame */
		cf->RTR = 1;

		/* check for optional DLC value for CAN 2.0B frames */
		if(cs[++idx] && (tmp = asc2nibble(cs[idx++])) <= CAN_MAX_DLEN) {
			cf->DLC = tmp;

			/* check for optional raw DLC value for CAN 2.0B frames */
			if ((tmp == CAN_MAX_DLEN) && (cs[idx++] == CC_DLC_DELIM)) {
				tmp = asc2nibble(cs[idx]);
				if ((tmp > CAN_MAX_DLEN) && (tmp <= CAN_MAX_RAW_DLC)) {
					CANTxFrame *ccf = (CANTxFrame *)cf;

					ccf->DLC = tmp;
				}
			}
		}
		return ret;
	}

	if (cs[idx] == CANID_DELIM) { /* CAN FD frame escape char '##' */

		maxdlen = CANFD_MAX_DLEN;
		ret = CANFD_MTU;

		/* CAN FD frame <canid>##<flags><data>* */
		if ((tmp = asc2nibble(cs[idx+1])) > 0x0F)
			return 0;

		//cf->flags = tmp;
		idx += 2;
	}

	for (i=0, dlen=0; i < maxdlen; i++){

		if(cs[idx] == DATA_SEPERATOR) /* skip (optional) separator */
			idx++;

		if(idx >= len) /* end of string => end of data */
			break;

		if ((tmp = asc2nibble(cs[idx++])) > 0x0F)
			return 0;
		cf->data8[i] = (tmp << 4);
		if ((tmp = asc2nibble(cs[idx++])) > 0x0F)
			return 0;
		cf->data8[i] |= tmp;
		dlen++;
	}
	cf->DLC = dlen;

	/* check for extra DLC when having a Classic CAN with 8 bytes payload */
	if ((maxdlen == CAN_MAX_DLEN) && (dlen == CAN_MAX_DLEN) && (cs[idx++] == CC_DLC_DELIM)) {
		unsigned char dlc = asc2nibble(cs[idx]);

		if ((dlc > CAN_MAX_DLEN) && (dlc <= CAN_MAX_RAW_DLC)) {
			CANTxFrame *ccf = (CANTxFrame *)cf;

			ccf->DLC = dlc;
		}
	}

	return ret;
}

static struct CANDriver *bus2can[] = {&CAND1, &CAND2};

void cmd_cansend(BaseSequentialStream *chp, int argc, char *argv[]) {

       (void)argv;
       int bus_num;
       int ret;
       int i;
       CANTxFrame cf;
   
       if (argc != 2) {
           chprintf(chp, "Usage: cansend bus_num[0/1] 123#BEEFDEAD12345678\r\n");
           return;
       }
   
       if (!strcmp(argv[0], "0")) {
           bus_num = 0;
       }
       else {
           bus_num = 1;
       }
   
       ret = parse_canframe(argv[1], &cf);

       ret = canTransmit(bus2can[bus_num], CAN_ANY_MAILBOX, &cf, TIME_IMMEDIATE);
       if (ret!= MSG_OK ){
         log(7, "canTransmit(%x,%d) at bus%d!\r\n", cf.IDE, cf.DLC, bus_num);
       }

       chprintf(chp, "cansend debug info:\r\n\tret=%d bus:%d, dlc=%d, IDE=%d,RTR=%d, %08x\r\n",
               ret, bus_num, cf.DLC, cf.IDE, cf.RTR, cf.EID);
       chprintf(chp, "\tdata: ");
       for(i=0; i<cf.DLC; i++)
           chprintf(chp, " %02X", cf.data8[i] );
           
       chprintf(chp, "\r\n");
}
