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

#define SHELL_WA_SIZE   THD_WORKING_AREA_SIZE(2048)

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

static THD_WORKING_AREA(my_wa, 1024);

static THD_FUNCTION(my, arg) {
  
  (void)arg;

  while (true) {
       chThdSleepMilliseconds(1000);
       palTogglePad(GPIOA, GPIOA_LED_B);
       
       log(LOG_LEVEL_DEBUG,    "%u LOG_LEVEL_DEBUG    %d[%d]\r\n", chVTGetSystemTimeX(), LOG_LEVEL_DEBUG,    log_level);
       log(LOG_LEVEL_INFO,     "%u LOG_LEVEL_INFO     %d[%d]\r\n", chVTGetSystemTimeX(), LOG_LEVEL_INFO,     log_level);
       log(LOG_LEVEL_WARNING,  "%u LOG_LEVEL_WARNING  %d[%d]\r\n", chVTGetSystemTimeX(), LOG_LEVEL_WARNING,  log_level);
       log(LOG_LEVEL_ERR,      "%u LOG_LEVEL_ERR      %d[%d]\r\n", chVTGetSystemTimeX(), LOG_LEVEL_ERR,      log_level);
  }

}

/*===========================================================================*/
/* Command line related.                                                     */
/*===========================================================================*/

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

//static thread_t *shelltp = NULL;

static const ShellCommand commands[] = {
      {"loglevel",     cmd_loglevel},
      {NULL, NULL}
      
};


static const ShellConfig shell_cfg1 = {
      (BaseSequentialStream *)&SD2,
      commands
};

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
   * Creates the example thread.
   */
  // Starting the transmitter and receiver threads
  chThdCreateStatic(my_wa, sizeof(my_wa), NORMALPRIO + 7,
                   my, (void *)NULL);

  while (TRUE) {  
    thread_t *shelltp = chThdCreateFromHeap(NULL, SHELL_WA_SIZE,
                                            "shell", NORMALPRIO + 1,
                                            shellThread, (void *)&shell_cfg1);
    chThdWait(shelltp);               /* Waiting termination.             */
    chThdSleepMilliseconds(1000);
    palTogglePad(GPIOA, GPIOA_LED_G);
  } 
}
