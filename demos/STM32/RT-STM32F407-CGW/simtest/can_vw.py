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
import cantools
from pprint import pprint
from vw import vw_crc
import logging

'''
dbcfile='../dbc/vw_mqb_2010.dbc'
db = cantools.database.load_file(dbcfile)
m = db.get_message_by_name('HCA_01')
pprint(m.signals)
'''

def sim_send(db):

    try:
        can_pt  = can.interface.Bus(bustype='socketcan', channel="can0", bitrate=500000)  #J533 Side/PowerTrain CAN
        can_ext = can.interface.Bus(bustype='socketcan', channel="can1", bitrate=500000)  #CAR Side/Extended CAN
    except OSError:
        print('Cannot find socketcan.')
        return False
                

    count = 0

    HCA_01 = db.get_message_by_name('HCA_01')
    HCA_count = 0
    TSK_06 = db.get_message_by_name('TSK_06')
    TSK_count = 0
    ESP_19 = db.get_message_by_name('ESP_19')
    LH_EPS_03 = db.get_message_by_name('LH_EPS_03')
    LH_EPS_count = 0

    
    while True:

        if True: ##100Hz

           data = ESP_19.encode( {
               'ESP_VL_Radgeschw_02': 100+count%2 if count>100000 else count/1000,
               'ESP_VR_Radgeschw_02': 100+count%2 if count>100000 else count/1000,
               'ESP_HL_Radgeschw_02': 100+count%2 if count>100000 else count/1000,
               'ESP_HR_Radgeschw_02': 100+count%2 if count>100000 else count/1000,
           })

           message = can.Message(arbitration_id=ESP_19.frame_id, data=data, is_extended_id=False)
           can_pt.send(message)
           #logging.info(db.decode_message(message.arbitration_id, message.data))

        if count%2: ##50Hz


           data = TSK_06.encode( {
               'CHECKSUM':0,
               'COUNTER': TSK_count,
               'TSK_Radbremsmom':0,
               'TSK_v_Begrenzung_aktiv':0,
               'TSK_Standby_Anf_ESP':0,
               'TSK_Freig_Verzoeg_Anf':0,
               'TSK_Limiter_ausgewaehlt': 0,
               'TSK_ax_Getriebe_02': 0,
               'TSK_Zwangszusch_ESP': 0,
               'TSK_zul_Regelabw': 0,
               'TSK_Status': 3  
           })

           TSK_count =  0 if TSK_count>=15 else TSK_count+1

           message = can.Message(arbitration_id=TSK_06.frame_id, data=data, is_extended_id=False)
           can_pt.send(message)
           logging.info(db.decode_message(message.arbitration_id, message.data))

        if count%2: ##50Hz

           data = LH_EPS_03.encode( {
               'CHECKSUM':0,
               'COUNTER':LH_EPS_count,
               'EPS_DSR_Status': 0,
               'EPS_HCA_Status': 3,  
               'EPS_Berechneter_LW': 0,
               'EPS_BLW_QBit': 0,
               'EPS_VZ_BLW': 0,
               'EPS_Lenkmoment': 0,
               'EPS_Lenkmoment_QBit': 0,
               'EPS_VZ_Lenkmoment': 0,
               'EPS_Lenkungstyp': 0,

           })

           message = can.Message(arbitration_id=LH_EPS_03.frame_id, data=data, is_extended_id=False)
           can_pt.send(message)
           LH_EPS_count =  0 if LH_EPS_count>=15 else LH_EPS_count+1
           #logging.info(db.decode_message(message.arbitration_id, message.data))

        if count%2==0: ##50Hz

           print("....", HCA_count)
           data = HCA_01.encode( {
               'CHECKSUM': 0,
               'COUNTER': HCA_count,
               'SET_ME_0X3': 3,
               'Assist_Torque': count%300,
           'Assist_Requested':1,
               'Assist_VZ':1, 
               'HCA_Available':1,
               'HCA_Standby':0,
               'HCA_Active':1,
               'SET_ME_0XFE':0xfe,
               'SET_ME_0X07':0x07
                   })

           crc = vw_crc(HCA_01.frame_id, d=data)

           data = HCA_01.encode( {
               'CHECKSUM': crc,
               'COUNTER': HCA_count,
               'SET_ME_0X3': 3,
               'Assist_Torque': count%300,
           'Assist_Requested':1,
               'Assist_VZ':1, 
               'HCA_Available':1,
               'HCA_Standby':0,
               'HCA_Active':1,
               'SET_ME_0XFE':0xfe,
               'SET_ME_0X07':0x07
                   })

           message = can.Message(arbitration_id=HCA_01.frame_id, data=data, is_extended_id=False)
           can_ext.send(message)
           HCA_count =  0 if HCA_count>=15 else HCA_count+1
           logging.info(db.decode_message(message.arbitration_id, message.data))
           
        time.sleep(0.01) # 100Hz
        count = count + 1

    return True

if __name__ == '__main__':

    parser = argparse.ArgumentParser(description="check if drop frame")
    #parser.add_argument('-d', '--device', help='CAN device', type=str, required=True)
    parser.add_argument('-f', '--dbcfile', help='DBC file', type=str, required=True)

    logging.basicConfig(level=logging.INFO)
    logging.info('\n\r CAN VW Test......')

    print('Bring up CAN0/1...')
    os.system("sudo ip link set can0 down")
    os.system("sudo ip link set can1 down")
    time.sleep(0.2)
    os.system("sudo ip link set can0 up type can bitrate 500000")
    os.system("sudo ip link set can1 up type can bitrate 500000")
    time.sleep(0.2)
    logging.info('Ready')


    args = vars(parser.parse_args())

    db = cantools.database.load_file(args['dbcfile'])

    sim_send(db=db)


    
