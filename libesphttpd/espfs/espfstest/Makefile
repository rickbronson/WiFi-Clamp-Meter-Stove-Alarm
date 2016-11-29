# search up in directory tree for  'esphttpdconfig.mk'
TOPDIR ?= $(shell TOPD=$$PWD ; until [ -f esphttpdconfig.mk -o "$$TOPD" = "/" ] ; do cd .. ; TOPD=$$PWD ; done ; echo $$TOPD)

# include common definitions (cross toolchain etc.)
include $(TOPDIR)/esphttpdconfig.mk

SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

$(TARGET): $(OBJS) Makefile $(TOPDIR)/esphttpdconfig.mk

espfstest: main.o espfs.o heatshrink_decoder.o
	$(CC) -o $@ $^

espfs.o: ../espfs.c
	$(CC) $(CFLAGS) -c $^ -o $@

heatshrink_decoder.o: ../heatshrink_decoder.c
	$(CC) $(CFLAGS) -c $^ -o $@

clean:
	rm -f *.o espfstest
