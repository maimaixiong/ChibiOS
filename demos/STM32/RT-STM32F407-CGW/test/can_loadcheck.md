#CAN LOAD CHECK

# Generate 85% can load

# check if drop frame

# Usage

- terminal A
	tmux kill-session -t cantest; tmux new-session -s cantest
- terminal B
	./can_loadcheck.py
	./canload_gen.sh
