all: pasco

CC ?= gcc 

install:  pasco
	cp pasco ../bin

pasco:  pasco.c
	${CC} -Wall -o pasco pasco.c -lm -lc

installwin:  pasco.exe
	pasco.exe ../bin

pasco.exe:  pasco.c
	gcc -DCYGWIN -o pasco.exe pasco.c -lm -lc;cp pasco.exe ../bin

clean: 
	rm -f pasco.exe pasco
