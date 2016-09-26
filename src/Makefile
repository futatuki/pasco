# $Id$

all: pasco

CC ?= gcc 
MINGW32_CC = mingw32-gcc

install:  pasco
	cp pasco ../bin

pasco:  pasco.c
	${CC} -Wall -o pasco pasco.c -lc

installwin:  pasco.exe
	cp pasco.exe ../bin

#pasco.exe:  pasco.c
#	gcc -DCYGWIN -o pasco.exe pasco.c -lc;cp pasco.exe ../bin

pasco.exe:  pasco.c
	${MINGW32_CC} -Wall -o pasco.exe pasco.c

clean: 
	rm -f pasco.exe pasco
