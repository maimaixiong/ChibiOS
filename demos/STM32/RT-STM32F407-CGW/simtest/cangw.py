#!/usr/bin/env python3
# usage:
# sudo modprobe vcan
# sudo ip link add vcan0 type vcan
# sudo ip link set vcan0 up

import sys
import zmq
import time
from collections import defaultdict, OrderedDict
import can

from selfdrive.boardd.boardd import can_list_to_can_capnp
#from selfdrive.car.toyota.toyotacan import make_can_msg
import cereal.messaging as messaging
from cereal.services import service_list

#def send(sendcan, addr, m):
#    packet = make_can_msg(addr, m, 0, False)
#    packets = can_list_to_can_capnp([packet], msgtype='sendcan')
#    sendcan.send(packets.to_bytes())
#

def recv_timeout(can, channel):
    received = False
    r = []
    t = time.time()

    while not received:
       c = messaging.recv_one_or_none(can)

       if c is not None:
          for msg in c.can:
             if msg.src == channel:
                r.append(msg)
                received = True

       if time.time() - t > 0.1:
          received = True

    return r


def can_bus_init(interface, channel):
    can.rc['interface'] = interface
    can.rc['channel'] = channel
    can_bus = can.interface.Bus()
    return can_bus

def main(rx_ch=0, bus_ch="vcan0", bus_inf="socketcan"):
    can_sub = messaging.sub_sock('can')
    bus = can_bus_init(bus_inf, bus_ch)
    #sendcan = messaging.pub_sock('sendcan')

    i = 0
    last_time = 0;
    while True:
        r = recv_timeout(can_sub, rx_ch)
        if len(r):
           i = i + 1
           for m in r:
               dat = []
               for b in m.dat:
                   dat.append(b)

               tx_msg = can.Message(arbitration_id = m.address, data=dat , is_extended_id=False)
               bus.send(tx_msg)
               delta = m.busTime - last_time
               if(delta < 0 ):
                   delta = m.busTime + 65535 - last_time

               print("can%d  %03X   [%d] "%(m.src,m.address,len(m.dat)), end=' ')
               last_time = m.busTime
               data_str = ' '.join(['%02X'% b for b in m.dat])
               print(data_str)
               time.sleep(0.005)

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("usage: %s channel [bus_channel] [bus_interface]"%sys.argv[0])
        exit(0)
    if len(sys.argv) > 3:
        main(int(sys.argv[1]), sys.argv[2], sys.argv[3])
    else:
        if(len(sys.argv)) > 2:
            main(int(sys.argv[1]), sys.argv[2])
        else:
            main(int(sys.argv[1]))




