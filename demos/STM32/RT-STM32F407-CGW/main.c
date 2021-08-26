/*
 * branch : minisys
 * board  : RT-STM32F407-CGW
 * author : dinglx@uincore.net (mmx)
 * rtos   : ChibiOS
*/

#include <stdio.h>
#include <string.h>

#include "ch.h"
#include "hal.h"
#include "rt_test_root.h"
#include "oslib_test_root.h"
#include "shell.h"
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

static int log_level = 6;

#define log(x, fmt, ...)  if(x<=log_level) chprintf( ((BaseSequentialStream *)&SD2), fmt, ## __VA_ARGS__ )

/*===========================================================================*/
/* Command line related.                                                     */
/*===========================================================================*/

#define SHELL_WA_SIZE   THD_WORKING_AREA_SIZE(2048)

/* Can be measured using dd if=/dev/xxxx of=/dev/null bs=512 count=10000.*/
static void cmd_write(BaseSequentialStream *chp, int argc, char *argv[]) {
      static uint8_t buf[] =
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
        
        (void)argv;
        if (argc > 0) {
                chprintf(chp, "Usage: write\r\n");
                    return;
                      
        }

        while (chnGetTimeout((BaseChannel *)chp, TIME_IMMEDIATE) == Q_TIMEOUT) {
        #if 1
            /* Writing in channel mode.*/
           chnWrite(&SDU1, buf, sizeof buf - 1);
        #else
           /* Writing in buffer mode.*/
           (void) obqGetEmptyBufferTimeout(&SDU1.obqueue, TIME_INFINITE);
           memcpy(SDU1.obqueue.ptr, buf, SERIAL_USB_BUFFERS_SIZE);
           obqPostFullBuffer(&SDU1.obqueue, SERIAL_USB_BUFFERS_SIZE);
        #endif
                                      
        }
        chprintf(chp, "\r\n\nstopped\r\n");
          
}

unsigned char asc2nibble(char c) {

    if ((c >= '0') && (c <= '9'))
        return c - '0';

    if ((c >= 'A') && (c <= 'F'))
        return c - 'A' + 10;

    if ((c >= 'a') && (c <= 'f'))
        return c - 'a' + 10;

    return 0; /* error */
                    
}

static void cmd_loglevel(BaseSequentialStream *chp, int argc, char *argv[]) {

       if (argc == 0) {
           chprintf(chp, "loglevel:%d\r\n", log_level);
           return;
       }
       
       if (argc == 1) {
           log_level = asc2nibble(argv[0][0]);
           chprintf(chp, "loglevel:%0d\r\n", log_level);
           return;
       }

       chprintf(chp, "\r\n");
}


static const ShellCommand commands[] = {
      {"write", cmd_write},
      {"loglevel", cmd_loglevel},
        {NULL, NULL}
      
};

static const ShellConfig shell_cfg = {
      (BaseSequentialStream *)&SDU1,
       commands
            
};


/*===========================================================================*/
/* Generic code.                                                             */
/*===========================================================================*/

#define LINE_LED        PAL_LINE(GPIOA, 6U)

/*
 *  Green LED blinker thread, times are in milliseconds.
 *  
 */
static THD_WORKING_AREA(waThread1, 128);
static THD_FUNCTION(Thread1, arg) {

      (void)arg;
      
      chRegSetThreadName("blinker");

      while (true) {

        systime_t time = serusbcfg.usbp->state == USB_ACTIVE ? 100 : 500;
        palClearLine(LINE_LED);
        chThdSleepMilliseconds(time);
        palSetLine(LINE_LED);
        chThdSleepMilliseconds(time);

      }
        
}

/*
 * Application entry point.
 */
int main(void) {

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
   * Activates the serial driver 2 using the driver default configuration.
   * PD5(TX) and PD6(RX) are routed to USART2.
   */
  sdStart(&SD2, NULL);
  palSetPadMode(GPIOD, 5, PAL_MODE_ALTERNATE(7));
  palSetPadMode(GPIOD, 6, PAL_MODE_ALTERNATE(7));

  /*
   *    * Initializes two serial-over-USB CDC drivers.
   *      
   */
  sduObjectInit(&SDU1);
  sduStart(&SDU1, &serusbcfg);

  /*
  * Activates the USB driver and then the USB bus pull-up on D+.
  * Note, a delay is inserted in order to not have to disconnect the cable
  * after a reset.
  */
  usbDisconnectBus(serusbcfg.usbp);
  chThdSleepMilliseconds(1500);
  usbStart(serusbcfg.usbp, &usbcfg);
  usbConnectBus(serusbcfg.usbp);

  /*
   * Shell manager initialization.
   * Event zero is shell exit.
   */
  shellInit();

  chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);

  while (true) {
    if (SDU1.config->usbp->state == USB_ACTIVE) {
        thread_t *shelltp = chThdCreateFromHeap(NULL, SHELL_WA_SIZE,
                                               "shell", NORMALPRIO + 1,
                                               shellThread, (void *)&shell_cfg);
        chThdWait(shelltp);               /* Waiting termination.             */
    }
    chThdSleepMilliseconds(1000);
  }
}
