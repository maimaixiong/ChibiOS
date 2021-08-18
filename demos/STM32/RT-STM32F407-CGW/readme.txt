*****************************************************************************
** ChibiOS/RT port for ARM-Cortex-M4 STM32F407.                            **
*****************************************************************************

** TARGET **

The demo runs on an ST STM32F4-CGW board.

** The Demo **


** Build Procedure **

The demo has been tested by using the free Codesourcery GCC-based toolchain
and YAGARTO. just modify the TRGT line in the makefile in order to use
different GCC toolchains.

** Notes **

Some files used by the demo are not part of ChibiOS/RT but are copyright of
ST Microelectronics and are licensed under a different license.
Also note that not all the files present in the ST library are distributed
with ChibiOS/RT, you can find the whole library on the ST web site:



** DING **

os/hal/ports/STM32/STM32F4xx/platform.mk

# Drivers compatible with the platform.
include $(CHIBIOS)/os/hal/ports/STM32/LLD/ADCv2/driver.mk
include $(CHIBIOS)/os/hal/ports/STM32/LLD/CANv1/driver.mk
include $(CHIBIOS)/os/hal/ports/STM32/LLD/CRYPv1/driver.mk
include $(CHIBIOS)/os/hal/ports/STM32/LLD/DACv1/driver.mk
include $(CHIBIOS)/os/hal/ports/STM32/LLD/DMAv2/driver.mk
include $(CHIBIOS)/os/hal/ports/STM32/LLD/EXTIv1/driver.mk
include $(CHIBIOS)/os/hal/ports/STM32/LLD/GPIOv2/driver.mk
include $(CHIBIOS)/os/hal/ports/STM32/LLD/I2Cv1/driver.mk
include $(CHIBIOS)/os/hal/ports/STM32/LLD/MACv1/driver.mk
include $(CHIBIOS)/os/hal/ports/STM32/LLD/OTGv1/driver.mk
include $(CHIBIOS)/os/hal/ports/STM32/LLD/QUADSPIv1/driver.mk
include $(CHIBIOS)/os/hal/ports/STM32/LLD/RTCv2/driver.mk
include $(CHIBIOS)/os/hal/ports/STM32/LLD/SPIv1/driver.mk
include $(CHIBIOS)/os/hal/ports/STM32/LLD/SDIOv1/driver.mk
include $(CHIBIOS)/os/hal/ports/STM32/LLD/SYSTICKv1/driver.mk
include $(CHIBIOS)/os/hal/ports/STM32/LLD/TIMv1/driver.mk
include $(CHIBIOS)/os/hal/ports/STM32/LLD/USARTv1/driver.mk
include $(CHIBIOS)/os/hal/ports/STM32/LLD/xWDGv1/driver.mk


./flash.sh build/ch.bin

                             http://www.st.com

## TEST

CGW4VW has two can interfaces: CAN1 and CAN2
- CAN1 connect to CAR BUS  (Bus1)
- CAN2 connect to J533 BUS (Bus0)

* TEST1 Rx from Bus0 TX to Bus1
    @Mac
      minicom -D /dev/tty.SLAB_USBtoUART (Baud 38400)


       ch>businfo 0
       businfo: 0
       -----------------------------------------------
       0       0x5A1(1441)     2155202 3       (709994:150554:866550)
       1       0x1B000315(452985621)   3774514 1       (4294967295:0:0)
       ------ total = 2 ---------------------------------

       ch>businfo 1
       businfo: 1
       -----------------------------------------------
       ------ total = 0 ---------------------------------

      @Ubuntu Terminal 1:
        STD FRAME
        candump any
         can0  5A1   [8]  11 22 33 44 55 66 77 88
         can1  5A1   [8]  11 22 33 44 55 66 77 88

        EXT FRAME
         can0  1B000315   [8]  11 22 33 44 55 66 77 88
         can1  1B000315   [8]  11 22 33 44 55 66 77 88

      @Ubuntu Terminal 2:
        STD FRAME
        cansend can0 5A1#11.2233.44556677.88
        EXT FRAME
        cansend can0 1B000315#1122334455667788


* TEST2 Rx from Bus1 TX to Bus0
    @Mac
      minicom -D /dev/tty.SLAB_USBtoUART (Baud 38400)


       ch>businfo 0
       businfo: 0
       -----------------------------------------------
       0       0x5A1(1441)     2155202 3       (709994:150554:866550)
       1       0x1B000315(452985621)   3774514 1       (4294967295:0:0)
       ------ total = 2 ---------------------------------

       ch>businfo 1
       businfo: 1
       -----------------------------------------------
       ------ total = 0 ---------------------------------

      @Ubuntu Terminal 1:
        STD FRAME
        candump any
         can1       315   [8]  01 23 45 67 BE EF DE AD
         can0       315   [8]  01 23 45 67 BE EF DE AD

        EXT FRAME
         can1  00000315   [8]  01 23 45 67 89 0A BC DE
         can0  00000315   [8]  01 23 45 67 89 0A BC DE

      @Ubuntu Terminal 2:
        STD FRAME
        cansend can1 315#01234567beefdead
        EXT FRAME
        cansend can1 00000315#01234567890abcde
# TEST3 TX from bus1 and bus0
        @CGW
        ch> cansend 0 123#01.23.45
        cansend debug info:
        ret=0 bus:0, dlc=3, IDE=0,RTR=0, 00000123
        data:  01 23 45

       ch> cansend 0 00000123#beefdead
       cansend debug info:
       ret=0 bus:0, dlc=4, IDE=1,RTR=0, 00000123
       data:  BE EF DE AD

       ch> cansend 1 51a#77889900
       cansend debug info:
       ret=0 bus:1, dlc=4, IDE=0,RTR=0, 0000051A
       data:  77 88 99 00

       ch> cansend 1 1B000315#0123456789abcdef
       ret=0 bus:1, dlc=8, IDE=1,RTR=0, 1B000315
       data:  01 23 45 67 89 AB CD EF


       @Ubuntu candump any

       can1       123   [3]  01 23 45
       can1  00000123   [4]  BE EF DE AD

       can0       51A   [4]  77 88 99 00
       can0  1B000315   [8]  01 23 45 67 89 AB CD EF

