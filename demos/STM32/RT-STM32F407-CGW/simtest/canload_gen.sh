# kill last time cangen
pkill -9 cangen
# 12% per line command
cangen can0 -g 1 -I 701 -L 1 -D i & #TO BE CHECK
cangen can0 -g 1 -I 702 -L 2 -D i & #TO BE CHECK
cangen can0 -g 2 -e -I 18FE0703 -L 8 -D i &

cangen can1 -g 1 -I 201 -L 1 -D i &
cangen can1 -g 1 -I 202 -L 2 -D i &
cangen can1 -g 2 -e -I 18FE0203 -L 8 -D i &

# display buload
canbusload can0@500000
