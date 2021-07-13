# DBCC

## download 
git clone https://github.com/maimaixiong/dbcc

## make
cd dbcc
make

## usage
./dbcc -h
./dbcc: [-] [-hvjgtxpkuDC] [-o dir] file*
dbcc - compile CAN DBC files to C code

Options:
	-      stop processing command line arguments
	-h     print out a help message and exit
	-v     make the program more verbose
	-g     print out the grammar used to parse the DBC files
	-t     add timestamps to the generated files
	-x     convert output to XML instead of the default C code
	-C     convert output to CSV instead of the default C code
	-b     convert output to BSM (beSTORM) instead of the default C code
	-j     convert output to JSON instead of the default C code
	-D     use 'double' for the encode/decode type messages
	-o dir set the output directory
	-p     generate only print code
	-k     generate only pack code
	-u     generate only unpack code
	-s     disable assert generation
	file   process a DBC file

Files must come after the arguments have been processed.

The parser combinator library (mpc) used in this program is licensed from
Daniel Holden, Copyright (c) 2013, under the BSD3 license
(see https://github.com/orangeduck/mpc/).

dbcc itself is licensed under the MIT license, Copyright (c) 2021, dinglx
Howe. (see https://github.com/maimaixiong/dbcc for the full program source).


## sample
./dbcc -o dbc dbc/vw.dbc
you got vw.h vw.c
