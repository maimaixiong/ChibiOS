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
from vw import vw_crc
from struct import pack
from random import *
import threading


def can_vw_gen(can_dev, start, stop):

    try:
        bus = can.interface.Bus(channel=can_dev, bustype='socketcan')
    except OSError:
        print('Cannot find socketcan.')
        return False
                
    print('Ready')

    count = 0 
    print("can_Dev:%s start:%d stop:%d"%(can_dev, start, stop))

    while True:
        address = randrange(start,stop)     
        b = pack('Q', count)
        d = list(b)
        count = count + 1
        crc = vw_crc(address, b)
        d[0] = crc
        msg = can.Message(arbitration_id=address,
                          data=d,
                          is_extended_id=False)
        try:
            bus.send(msg)
        except can.CanError:
            print("Message NOT send", msg)

        if count%5000==0:
            print("[%d,%d] count:{%d}"%(start, stop, count))

        time.sleep(0.001)

    return True

if __name__ == '__main__':

    parser = argparse.ArgumentParser(description="check if drop frame")
    parser.add_argument('-d', '--device', help='CAN device', type=str, required=True)
    parser.add_argument('-n', '--number', help='send thread number', type=int, required=True)

    args = vars(parser.parse_args())

    dev = args['device']
    num = args['number']
    addr_max = 0x7ff

    sz = int(addr_max/num)

    thread_list = []

    for i in range(num):
        t = threading.Thread(target=can_vw_gen, args=(dev, i*sz , (i+1)*sz))
        t.start()
        thread_list.append(t)
     

    for i in range(num):
        thread_list[i].join() 
