# kill last time cangen
pkill -9 cangen
# 12% per line command
cangen can0 -g 1 -I 701 -L 1 -D i &
cangen can0 -g 1 -I 702 -L 2 -D i &
cangen can0 -g 1 -I 703 -L 3 -D i &

cangen can1 -g 1 -I 201 -L 1 -D i &
cangen can1 -g 1 -I 202 -L 2 -D i &
cangen can1 -g 1 -I 203 -L 3 -D i &

# display buload
canbusload can0@500000
