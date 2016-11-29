TARGET=heatshrink

# search up in directory tree for  'esphttpdconfig.mk'
TOPDIR ?= $(shell TOPD=$$PWD ; until [ -f esphttpdconfig.mk -o "$$TOPD" = "/" ] ; do cd .. ; TOPD=$$PWD ; done ; echo $$TOPD)

# include common definitions (cross toolchain etc.)
include $(TOPDIR)/esphttpdconfig.mk

SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

$(TARGET): $(OBJS) Makefile $(TOPDIR)/esphttpdconfig.mk

PROJECT = heatshrink

all:
	@echo "For tests, make test_heatshrink_dynamic (default) or change the"
	@echo "config.h to disable static memory and build test_heatshrink_static."
	@echo "For the standalone command-line tool, make heatshrink."

${TARGET}: heatshrink.c

OBJS= 	heatshrink_encoder.o \
	heatshrink_decoder.o \

heatshrink: ${OBJS}
test_heatshrink_dynamic: ${OBJS} test_heatshrink_dynamic_theft.o
test_heatshrink_static: ${OBJS}

*.o: Makefile heatshrink_config.h

heatshrink_decoder.o: heatshrink_decoder.h heatshrink_common.h
heatshrink_encoder.o: heatshrink_encoder.h heatshrink_common.h

tags: TAGS

TAGS:
	etags *.[ch]

diagrams: dec_sm.png enc_sm.png

dec_sm.png: dec_sm.dot
	dot -o $@ -Tpng $<

enc_sm.png: enc_sm.dot
	dot -o $@ -Tpng $<

clean:
	rm -f ${PROJECT} test_heatshrink_{dynamic,static} *.o *.core {dec,enc}_sm.png TAGS
