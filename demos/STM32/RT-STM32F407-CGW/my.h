#include <stdlib.h>

#include "ch.h"
#include "hal.h"

#include "chprintf.h"


extern int log_level;
#define log(x, fmt, ...)  if(x<=log_level) chprintf( ((BaseSequentialStream *)&SD2), fmt, ## __VA_ARGS__ )

/*
 * CAN ID:
 *      bit31: EVENT
 *      bit30: EXTEND ID(29bit)
 *      bit29: RTR
 *      bit28..0 can_id
 */
#define CID_TYPE_MASK_EVENT   0x80
#define CID_TYPE_MASK_EXT     0x40
#define CID_TYPE_MASK_RTR     0x20
#define CID_TYPE_MASK_CH      0x0f

struct myRxMsg_s {
    int can_bus; //1: J533  2:CAR
    CANRxFrame fr;
};

typedef struct myRxMsg_s myRxMsg_t;

void can_init(void);
