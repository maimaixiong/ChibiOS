# kill last time cangen
pkill -9 cangen
# 12% per line command
cangen can1 -g 2 -I 401 -L 1 -D i &
cangen can1 -g 2 -I 402 -L 2 -D i &
#cangen can1 -g 1 -I 403 -L 3 -D i &
#cangen can1 -g 1 -I 404 -L 4 -D i &
#cangen can1 -g 1 -I 405 -L 5 -D i &
#cangen can1 -g 1 -I 406 -L 6 -D i &
#cangen can1 -g 1 -I 407 -L 7 -D i &
#cangen can1 -g 1 -I 408 -L 8 -D i &

# display buload
canbusload can1@500000
