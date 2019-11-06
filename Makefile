CFLAGS = -Wall -m64 -ggdb -I. -fPIC
LFLAGS = ${CFLAGS} -fPIC -shared

CC = cc
TARGET = libreadini.so
FNAME = libreadini

all : ${TARGET} ritest

ritest : ritest.c ${TARGET}
	$(CC) ${CFLAGS} -L. -o ritest ritest.c -Wl,-R -Wl,. -lreadini

${TARGET} : readini.c
	$(CC) ${LFLAGS} -o ${TARGET} readini.c

clean :
	rm -f ritest ${TARGET}

install :
	install -D --mode=755 libreadini.so /usr/local/lib
	install -D --mode=755 readini.h     /usr/local/include

uninstall :
	rm -f /usr/local/lib/libreadini.so
	rm -f /usr/local/include/readini.h
