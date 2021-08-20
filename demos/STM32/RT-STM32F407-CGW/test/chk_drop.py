#!/usr/bin/env python3
#
# simple_rx_test.py
# 
# This is simple CAN receive python program. All messages received are printed out on screen.
# For use with PiCAN boards on the Raspberry Pi
# http://skpang.co.uk/catalog/pican2-canbus-board-for-raspberry-pi-2-p-1475.html
#
# Make sure Python-CAN is installed first http://skpang.co.uk/blog/archives/1220
#
# 01-02-16 SK Pang
#
#
#

import can
import time
import os
import struct


print('\n\rCAN Rx and Check Drop Frame')
#print('Bring up CAN0....')
#os.system("sudo /sbin/ip link set can0 up type can bitrate 500000")
#time.sleep(0.1) 

try:
    bus = can.interface.Bus(channel='can0', bustype='socketcan')
except OSError:
    print('Cannot find socketcan.')
    exit()
                
print('Ready')

last_dict = { 0x401: -1, 0x402: -1 }

count = 0
drop_count = 0

try:
    while True:
        message = bus.recv()    # Wait until a message is received.
        count = count + 1

        h = '{0:d} {1:d} {2:f}% '.format(drop_count, count,  drop_count*100.0/count)

        if message.arbitration_id == 0x401:

            d = struct.unpack('B', message.data)
            exp_val = last_dict[0x401] + 1
            if exp_val > 0xff:
                exp_val = 0
            c = '{0:f} {1:x} {2:x} '.format(message.timestamp, message.arbitration_id, message.dlc)
            s=''
            for i in range(message.dlc ):
                s +=  '{0:x} '.format(message.data[i])

            v=''

            if d[0] != exp_val and last_dict[0x401] != -1:
                v= '!!! {0:x} {1:x} '.format(d[0], last_dict[0x401])
                drop_count = drop_count + 1

            print(' {}'.format(h+c+s+v))
                        
            last_dict[0x401] = d[0]
                                                                                                                
        if message.arbitration_id == 0x402:

            d = struct.unpack('H', message.data)
            exp_val = last_dict[0x402] + 1
            if exp_val > 0xffff:
                exp_val = 0

            c = '{0:f} {1:x} {2:x} '.format(message.timestamp, message.arbitration_id, message.dlc)
            s=''
            for i in range(message.dlc ):
                s +=  '{0:x} '.format(message.data[i])

            v=''

            if d[0] != exp_val and last_dict[0x402] != -1:
                v= '!!! {0:x} {1:x} '.format(d[0], last_dict[0x402])
                drop_count = drop_count + 1

            print(' {}'.format(h+c+s+v))
                        
            last_dict[0x402] = d[0]

except KeyboardInterrupt:
    #Catch keyboard interrupt
    #os.system("sudo /sbin/ip link set can0 down")
    print('\n\rKeyboard interrtupt')    
