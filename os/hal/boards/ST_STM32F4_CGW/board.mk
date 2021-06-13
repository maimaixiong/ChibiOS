# List of all the board related files.
BOARDSRC = $(CHIBIOS)/os/hal/boards/ST_STM32F4_CGW/board.c

# Required include directories
BOARDINC = $(CHIBIOS)/os/hal/boards/ST_STM32F4_CGW

# Shared variables
ALLCSRC += $(BOARDSRC)
ALLINC  += $(BOARDINC)
