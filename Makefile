CFLAGS = -D_POSIX_C_SOURCE=200809L -Os -pedantic -std=c99 -Wall
CC = gcc

SRC = flashcards.c util.c
OBJ = ${SRC:.c=.o}

PREFIX = /usr/local
SHAREDIR = ${PREFIX}/share/flashcards

all: flashcards

clean:
	rm -f flashcards ${OBJ}

flashcards: ${OBJ}
	${CC} -o $@ $^

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f flashcards ${DESTDIR}${PREFIX}/bin
	chmod +x ${DESTDIR}${PREFIX}/bin/flashcards
	mkdir -p ${DESTDIR}${SHAREDIR}
	cp -f flashcards.tex output.tex ${DESTDIR}${SHAREDIR}

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/flashcards
	rm -rf ${DESTDIR}${SHAREDIR}

${OBJ}: ${SRC}

.PHONY: all clean install uninstall
