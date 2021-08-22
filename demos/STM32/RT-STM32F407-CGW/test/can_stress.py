#!/usr/bin/env python

import time
import os
from subprocess import Popen
import subprocess
import can
import struct

commands = [
    'cangen can0 -g 1 -I 101 -L 1 -D i',
    'cangen can0 -g 1 -I 102 -L 2 -D i',
    'cangen can0 -g 1 -I 103 -L 3 -D i',
#    'cangen can0 -g 1 -I 104 -L 4 -D i',
#    'cangen can0 -g 1 -I 105 -L 5 -D i',
    'cangen can1 -g 1 -I 201 -L 1 -D i',
    'cangen can1 -g 1 -I 202 -L 2 -D i',
    'cangen can1 -g 1 -I 203 -L 3 -D i',
#   'cangen can0 -g 1 -I 204 -L 4 -D i',
#    'cangen can0 -g 1 -I 205 -L 5 -D i',
]

print('\n\r CAN Stress Test......')
print('Bring up CAN0/1...')
os.system("sudo ip link set can0 down")
os.system("sudo ip link set can1 down")
time.sleep(0.2)
os.system("sudo ip link set can0 up type can bitrate 500000")
os.system("sudo ip link set can1 up type can bitrate 500000")
time.sleep(0.2)
print('Ready')

bus1_dict = { 
        0x101: {'count':0, 'last_count':-1}, 
        }

bus2_dict = { 
        0x201: {'count':0, 'last_count':-1}, 
        }

try:

    try:
        bus1 = can.interface.Bus(channel='can0', bustype='socketcan')
        bus2 = can.interface.Bus(channel='can1', bustype='socketcan')
    
    except OSError:
        print('Cannot find socketcan.')
        exit()
    
    # run in parallel
    #processes = [Popen(cmd, shell=True) for cmd in commands]
    processes = [Popen(cmd, shell=True, stdout=subprocess.PIPE) for cmd in commands]
    # do other things here..
    # wait for completion
    #for p in processes: 
    #    output = p.stdout.read()
    #    print(output)
    #    #p.wait()
    
    #while True:
    #    print(".")
    #    output = processes[3].stdout.read()
    #    print(output)
    
    print(bus1_dict.keys())
    print(bus2_dict.keys())

    while True:
        m1 = bus1.recv()
        #m2 = bus2.recv()
   
        addr = m1.arbitration_id
        if addr in bus1_dict.keys():

            if addr == 0x101:
                d = struct.unpack('B', m1.data)
                bus1_dict[addr]['count'] = bus1_dict[addr]['count'] + 1
                
                exp_count = bus1_dict[addr]['last_count'] 
                exp_count = exp_count + 1
                if exp_count> 0xff: exp_count = 0
                
                bus1_dict[addr]['last_count'] = d[0]

                if d[0] != exp_count and bus1_dict[addr]['last_count'] != -1:
                    print("!!!")

                print(d[0], exp_count, bus1_dict)

                
            

        #if m2.arbitration_id in bus2_dict.keys():
        #    print("bus2:",m1)



except KeyboardInterrupt:
    #Catch keyboard interrupt
    os.system("sudo ip link set can0 down")
    os.system("sudo ip link set can0 down")
    print('\n\rKeyboard interrtupt')

