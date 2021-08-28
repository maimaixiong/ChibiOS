#CAN LOAD CHECK

# Generate 85% can load

# check if drop frame

# replay real data CAN data (VW J533)

 - rx from bus0: J533
 - rx from bus2: CAR
 - tx to bus0: J533 by using 128
 - tx to bus2: J533 by using 130
 - VW J533 read data route is '7bae7f72b920c617|2021-07-25--23-28-42'
 - replay command:
'''
cd ~/openpilot/tools/replay/  
./unlogger.py '7bae7f72b920c617|2021-07-25--23-28-42' ~/Downloads/vwj533_data/
'''

# Usage
- terminal A

cd ~/openpilot/tools/replay/  
./unlogger.py '7bae7f72b920c617|2021-07-25--23-28-42' ~/Downloads/vwj533_data/

- terminal B
	tmux kill-session -t cantest; tmux new-session -s cantest

- terminal C
	./can_loadcheck.py
	./canload_gen.sh
