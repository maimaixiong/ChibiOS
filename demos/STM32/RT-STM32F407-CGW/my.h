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
extern mutex_t mtx;
extern volatile bool LaneAssist;
extern systime_t ot;
extern bool hack_mode;
extern int hcaEnabledCount_max, hcaSameTorqueCount_max;

//SerialPort
//#define log(x, fmt, ...)  if(x<=log_level) chprintf( ((BaseSequentialStream *)&SD2), fmt, ## __VA_ARGS__ )
#define log(x, fmt, ...)  if(x<=log_level) chprintf( ((BaseSequentialStream *)&SDU1), fmt, ## __VA_ARGS__ )

struct myRxMsg_s {
    int can_bus; //1: J533  2:CAR
    CANRxFrame fr;
    systime_t timestamp;
};

typedef struct myRxMsg_s myRxMsg_t;

typedef struct {
   volatile uint32_t w_ptr;
   volatile uint32_t r_ptr;
   uint32_t fifo_size;
   CANTxFrame *elems;
} can_ring;

#define CAN_RX_MSG_SIZE 2048 
#define DEFAULT_TIMEOUT 1000000*10 //20 sec

extern int can_rx_cnt[];
extern int can_tx_cnt[];
extern int can_txd_cnt[];
extern int can_err_cnt[];


unsigned char asc2nibble(char c);
void can_init(void);
void candump(myRxMsg_t *pMsg);
//int putMailMessage(int can_bus, CANRxFrame* pf);
int getMailMessage(myRxMsg_t *pMsg);

void vw_crc_init(void);
uint8_t vw_crc(uint64_t d, int l);
void hca_process(CANTxFrame *txmsg);
void canframe_copy( CANTxFrame *tx, CANRxFrame *rx );


