#include <stdlib.h>
#include <string.h>

#include "ch.h"
#include "hal.h"

#include "chprintf.h"
#include "usbcfg.h"


#define LOG_LEVEL_EMERG   0  /* systemis unusable */
#define LOG_LEVEL_ALERT   1  /* actionmust be taken immediately */
#define LOG_LEVEL_CRIT    2  /*critical conditions */
#define LOG_LEVEL_ERR     3  /* errorconditions */
#define LOG_LEVEL_WARNING 4  /* warning conditions */
#define LOG_LEVEL_NOTICE  5  /* normalbut significant */
#define LOG_LEVEL_INFO    6  /* informational */
#define LOG_LEVEL_DEBUG   7  /*debug-level messages */

extern int log_level;
extern bool LaneAssist;
extern systime_t ot;
extern bool hack_mode;

//SerialPort
//#define log(x, fmt, ...)  if(x<=log_level) chprintf( ((BaseSequentialStream *)&SD2), fmt, ## __VA_ARGS__ )
#define log(x, fmt, ...)  if(x<=log_level) chprintf( ((BaseSequentialStream *)&SDU1), fmt, ## __VA_ARGS__ )

struct myRxMsg_s {
    int can_bus; //1: J533  2:CAR
    CANRxFrame fr;
    systime_t timestamp;
};

typedef struct myRxMsg_s myRxMsg_t;


#define CAN_RX_MSG_SIZE 32
#define DEFAULT_TIMEOUT 1000000*20 //20 sec


void can_init(void);
void candump(myRxMsg_t *pMsg);
//int putMailMessage(int can_bus, CANRxFrame* pf);
int getMailMessage(myRxMsg_t *pMsg);

void vw_crc_init(void);
uint8_t vw_crc(uint64_t d, int l);


