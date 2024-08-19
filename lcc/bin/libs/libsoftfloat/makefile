#
# Makefile for Libs

# LCCDIR =../..
CC = ${LCCDIR}/lcc
AS = ${LCCDIR}/../../binutils/bin/as
AR = ${LCCDIR}/../../binutils/bin/ar

CFLAGS = -S -D__TCS_V2__ -w -v


BIN = ${CURDIR}.a

.PHONY:		all clean

all:		$(BIN)

softfloat.s:softfloat.c
	$(CC) $(CFLAGS) $<

softfloat.o:softfloat.s
	$(AS) -o $@ $<

$(BIN):softfloat.o
	$(AR) -rsv $@ $^

clean:
		rm -f *~ softfloat.o softfloat.s $(BIN)
