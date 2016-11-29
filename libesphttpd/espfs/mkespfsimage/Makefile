TARGET=mkespfsimage

# search up in directory tree for  'esphttpdconfig.mk'
TOPDIR ?= $(shell TOPD=$$PWD ; until [ -f esphttpdconfig.mk -o "$$TOPD" = "/" ] ; do cd .. ; TOPD=$$PWD ; done ; echo $$TOPD)

# include common definitions (cross toolchain etc.)
include $(TOPDIR)/esphttpdconfig.mk

SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
CFLAGS = $(HOSTCFLAGS)
CC = $(HOSTCC)

$(TARGET): $(OBJS) Makefile $(TOPDIR)/esphttpdconfig.mk
	$(CC) -g -ggdb -o $@ $(OBJS)

clean:
	rm -f $(TARGET) $(OBJS)