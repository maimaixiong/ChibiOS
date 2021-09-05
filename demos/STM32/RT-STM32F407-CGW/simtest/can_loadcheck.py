#!/usr/bin/env python
# coding=utf-8
# author: dinglx    
# date: 2021-08-20
# Usage:
#    run several commands in tmux panel, every window have 4 panes
#    $ pip3 install libtmux
#    $ python can_loadcheck.py


import libtmux
import math
import os
import logging
import time
import subprocess
from subprocess import Popen

def attach_session(session_name, commands):
    '''
        create session with 2*2 panes and run commands
    :param session_name: tmux session name
    :param commands: bash commands
    :return:
    '''
    logging.info(commands)
    # pane_NUM = 3
    # WINDOW_NUM = int(math.ceil(len(commands)/4.0))  # in python3, we can use 4 also

    server = libtmux.Server()

    session = server.find_where({'session_name':session_name})
    #session = server.new_session(session_name)

    # create windows
    windows = []
    panes = []


    for i in range(len(commands)):
        # create window to store 4 panes
        if i % 4 == 0:
            win = session.new_window(attach=False, window_name="win"+str(int(i/4)))
            windows.append(win)

            # tmux_args = ('-h',)
            win.cmd('split-window', '-h')
            win.cmd('split-window', '-f')
            win.cmd('split-window', '-h')

            panes.extend(win.list_panes())

        panes[i].send_keys(commands[i])

    logging.info(panes)
    logging.info(windows)
    session.kill_window("bash")

if __name__ == '__main__':

    logging.basicConfig(level=logging.INFO)
    logging.info('\n\r CAN Stress Test......')
    print('Bring up CAN0/1...')
    os.system("sudo ip link set can0 down")
    os.system("sudo ip link set can1 down")
    time.sleep(0.2)
    os.system("sudo ip link set can0 up type can bitrate 500000")
    os.system("sudo ip link set can1 up type can bitrate 500000")
    time.sleep(0.2)
    logging.info('Ready')

    #processes = [Popen(cmd, shell=True, stdout=subprocess.PIPE) for cmd in commands]
    


    commands = [
                'candump can0',
                'candump can1',
                'cansniffer can0',
                'cansniffer can1',
                './can_chkseq.py -i 0x701 -d can1 -f B -m 0xff',
                './can_chkseq.py -i 0x702 -d can1 -f H -m 0xffff',
                './can_chkseq.py -i 0x201 -d can0 -f B -m 0xff',
                './can_chkseq.py -i 0x202 -d can0 -f H -m 0xffff',
                './cangw.py 128 can1',
                './cangw.py 0 can0',
                './can_chkseq_hca.py'
            ]

    #os.system("tmux kill-session -t canview")
    attach_session("cantest", commands)

