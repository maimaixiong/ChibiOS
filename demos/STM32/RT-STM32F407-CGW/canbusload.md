# CANBUS Stress Test

## generate CANBUS load 80%+ using cangen

```
cat canload_gen.sh 
# kill last time cangen
pkill -9 cangen
# 12% per line command
cangen can1 -g 1 -I 401 -L 1 -D i &
cangen can1 -g 1 -I 402 -L 1 -D i &
cangen can1 -g 1 -I 403 -L 1 -D i &
cangen can1 -g 1 -I 404 -L 1 -D i &
cangen can1 -g 1 -I 405 -L 1 -D i &
cangen can1 -g 1 -I 406 -L 1 -D i &
cangen can1 -g 1 -I 407 -L 1 -D i &
#cangen can1 -g 1 -I 408 -L 1 -D i &

# display buload
canbusload can1@500000
```

## python-can 

- github 
```
 git clone https://github.com/hardbyte/python-can
```
- install python-can
```
pip install python-can
```
- can.viewer https://python-can.readthedocs.io/en/master/scripts.html
```
python -m can.viewer  -c can0 -b 500000 -i socketcan

python -m can.viewer  -c can0 -b 500000 -i socketcan -d "403:<HB:100.0:1"

```



## How to test CAN Message forward drop frame?

- terminal A:
run canload_gen.sh 
generate can1 a lof message!

- terminal B:
candump can0,401:7ff

