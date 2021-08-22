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
import argparse
from ast import literal_eval


def can_checkseq(can_dev, can_id, format_str='B', maxval=0xff):

    try:
        bus = can.interface.Bus(channel=can_dev, bustype='socketcan')
    except OSError:
        print('Cannot find socketcan.')
        return False
                
    print('Ready')

    count = 0
    drop_count = 0
    last_val = -1
    
    while True:

        message = bus.recv()    # Wait until a message is received.
        count = count + 1
    
        h = '{0:s} {1:d} {2:d} {3:f}% '.format(can_dev, drop_count, count,  drop_count*100.0/count)
    
        if message.arbitration_id == can_id:
    
            d = struct.unpack(format_str, message.data)
            exp_val = last_val + 1
            if exp_val > maxval:
                exp_val = 0
            #c = '{0:f} {1:x} {2:x} '.format(message.timestamp, message.arbitration_id, message.dlc)
            c = '{0:x} {1:x} '.format(message.arbitration_id, message.dlc)
            s=''
            for i in range(message.dlc ):
                s +=  '{0:x} '.format(message.data[i])
    
            v=''
    
            if d[0] != exp_val and last_val != -1:
                v= '!!! {0:x} {1:x} {2:x}'.format(d[0], exp_val, last_val)
                drop_count = drop_count + 1
    
            print(' {}'.format(h+c+s+v))
                        
            last_val = d[0]

    return True

if __name__ == '__main__':

    parser = argparse.ArgumentParser(description="check if drop frame")
    parser.add_argument('-i', '--id', help='CAN ID', type=str, required=True)
    parser.add_argument('-d', '--device', help='CAN device', type=str, required=True)
    parser.add_argument('-f', '--format_str', help='decode format string',type=str, required=True)
    parser.add_argument('-m', '--maxval', help='maxval', type=str, required=True)

    args = vars(parser.parse_args())

    can_checkseq(can_dev=args['device'], can_id=literal_eval(args['id']), format_str=args['format_str'], maxval=literal_eval(args['maxval']))


    
