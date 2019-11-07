CFLAGS = -Wall -m64 -ggdb -I. -fPIC
LFLAGS = ${CFLAGS} -fPIC -shared

CC = cc
TARGET = libreadini.so
FNAME = libreadini

all : ${TARGET}

${TARGET} : readini.c
	$(CC) ${LFLAGS} -o ${TARGET} readini.c

clean :
	rm -f ritest ${TARGET}

install :
	install -D --mode=755 libreadini.so /usr/lib
	install -D --mode=755 readini.h     /usr/local/include

uninstall :
	rm -f /usr/lib/libreadini.so
	rm -f /usr/local/include/readini.h
