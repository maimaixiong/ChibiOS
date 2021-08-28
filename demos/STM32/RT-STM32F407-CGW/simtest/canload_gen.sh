# kill last time cangen
pkill -9 cangen
# 12% per line command
cangen can0 -g 2 -I 1 -L 1 -D i &
cangen can0 -g 2 -I 2 -L 2 -D i &
cangen can0 -g 2 -I 3 -L 3 -D i &

cangen can1 -g 2 -I 201 -L 1 -D i &
cangen can1 -g 2 -I 202 -L 2 -D i &
cangen can1 -g 2 -I 203 -L 3 -D i &

# display buload
canbusload can0@500000
