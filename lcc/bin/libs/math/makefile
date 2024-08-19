#
# Makefile for Libs

CC = ${LCCDIR}/lcc
AS = ${LCCDIR}/../../binutils/bin/as
AR = ${LCCDIR}/../../binutils/bin/ar

CFLAGS = -c -D__TCS_V2__ -v


CSRCS = $(wildcard *.c)
ASRCS = $(patsubst %.c,%.s,$(CSRCS))
OSRCS = $(patsubst %.c,%.o,$(CSRCS))
BIN = ${CURDIR}.a

.PHONY:		all clean

all:		$(BIN)

%.s:%.c
	$(CC) $(CFLAGS) $<

$(BIN):$(OSRCS)
	$(AR) -rsv $@ $^

clean:
		rm -f *~ $(OSRCS) $(ASRCS) $(BIN)
