# Final Test

## Hardware Test

- Power: Pin 12/14/16 to +12V, Pin 1 to GND
- CAN1: J533 Side(Pin 8,10)  120Ohm
- CAN2: Car  Side(Pin 4, 6)  120Ohm

## Software

- CAN1: rx from J533 side and to CAR(CAN2)    STD 11bit/EXTENDED 29bit
- CAN2: rx from CAR  side and to J533(CAN1)   STD 11bit/EXTENDED 29bit
- J533 side is Powertrain Bus
- CAR side is Extend Bus(Camera/ADAS)
