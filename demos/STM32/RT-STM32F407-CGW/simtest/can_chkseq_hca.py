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


def can_checkseq(can_dev="can0", can_id=0x126, maxval=0xf):

    try:
        bus = can.interface.Bus(channel=can_dev, bustype='socketcan')
    except OSError:
        print('Cannot find socketcan.')
        return False
                
    print('Ready')

    count = 0
    drop_count = 0
    last_val = -1

    delta_min = 0xffff
    delta_max = 0
    
    last_timestamp = -1
    

    while True:

        message = bus.recv()    # Wait until a message is received.
        count = count + 1
    
        h = '{0:s} {1:d} {2:d} {3:f}% '.format(can_dev, drop_count, count,  drop_count*100.0/count)
    
        if message.arbitration_id == can_id:
            
            if last_timestamp > 0:
                delta = message.timestamp - last_timestamp 
                if delta<delta_min:
                    delta_min = delta
                if delta>delta_max:
                    delta_max = delta
            else:
                delta = -1

            last_timestamp = message.timestamp

            d = message.data[1]&0x0f #struct.unpack(format_str, message.data)
            exp_val = last_val + 1
            if exp_val > maxval:
                exp_val = 0
            #c = '{0:f} {1:x} {2:x} '.format(message.timestamp, message.arbitration_id, message.dlc)
            c = '{0:03x} {1:01d} '.format(message.arbitration_id, message.dlc)
            s=''
            for i in range(message.dlc ):
                s +=  '{0:02x} '.format(message.data[i])
    
            v=''
    
            if d != exp_val and last_val != -1:
                v= '!!!---!!! get:{0:02x} exp:{1:02x} last:{2:02x}'.format(d, exp_val, last_val)
                drop_count = drop_count + 1

            dt='delta:{0:0.6f} ({1:0.6f},{2:0.6f})'.format(delta, delta_min, delta_max)
    
            print(' {}'.format(h+c+s+v+dt))
                        
            last_val = d

    return True

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="check if drop frame")
    parser.add_argument('-d', '--device', help='CAN Device', type=str, required=True)
    args = vars(parser.parse_args())

    can_checkseq(can_dev=args['device'], can_id=0x126, maxval=0xf)


    
