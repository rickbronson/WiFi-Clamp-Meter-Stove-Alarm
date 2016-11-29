# --------------- esphttpd config options ---------------

# If GZIP_COMPRESSION is set to "yes" then the static css, js, and html files will be compressed with gzip before added to the espfs image
# and will be served with gzip Content-Encoding header.
# This could speed up the downloading of these files, but might break compatibility with older web browsers not supporting gzip encoding
# because Accept-Encoding is simply ignored. Enable this option if you have large static files to serve (for e.g. JQuery, Twitter bootstrap)
# By default only js, css and html files are compressed.
# If you have text based static files with different extensions what you want to serve compressed then you will need to add the extension to the following places:
# - Add the extension to this Makefile at the webpages.espfs target to the find command
# - Add the extension to the gzippedFileTypes array in the user/httpd.c file
#
# Adding JPG or PNG files (and any other compressed formats) is not recommended, because GZIP compression does not works effectively on compressed files.

#Static gzipping is disabled by default.
GZIP_COMPRESSION ?= no

# If COMPRESS_W_YUI is set to "yes" then the static css and js files will be compressed with yui-compressor
# This option works only when GZIP_COMPRESSION is set to "yes"
# http://yui.github.io/yuicompressor/
#Disabled by default.
COMPRESS_W_YUI ?= no
YUI-COMPRESSOR ?= /usr/bin/yui-compressor

#If USE_HEATSHRINK is set to "yes" then the espfs files will be compressed with Heatshrink and decompressed
#on the fly while reading the file. Because the decompression is done in the esp8266, it does not require
#any support in the browser.
USE_HEATSHRINK ?= yes

# base directory of the ESP8266 SDK package, absolute
SDK_BASE ?= $(HOME)/boards/esp8266/esp-open-sdk/esp_iot_sdk_v1.4.0

# Base directory for the compiler.
XTENSA_TOOLS_ROOT=$(HOME)/boards/esp8266/esp-open-sdk/xtensa-lx106-elf/bin

#Esptool.py path and port
MYESPTOOL ?= $(HOME)/boards/esp8266/esptool.py/esptool.py
ESPPORT		?= /dev/ttyUSB0
#ESPDELAY indicates seconds to wait between flashing the two binary images
ESPDELAY	?= 3
ESPBAUD		?= 460800

#You can build this example in three ways, comment 2 of the lines below
# 'separate' - Separate espfs and binaries, no OTA upgrade
# 'combined' - Combined firmware blob, no OTA upgrade
# 'ota' - Combined firmware blob with OTA upgrades.

OUTPUT_TYPE=separate
#OUTPUT_TYPE=combined
#OUTPUT_TYPE=ota

# --------------- End of user config ---------------

# select which tools to use as compiler, librarian and linker
CC		:= $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-gcc
AR		:= $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-ar
LD		:= $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-gcc
OBJCOPY	:= $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-objcopy
OBJDUMP	:= $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-objdump

# esptools needs path to tools
ESPTOOL		?= PATH=$(XTENSA_TOOLS_ROOT):$(PATH) $(MYESPTOOL)

# various paths from the SDK used in this project
SDK_LIBDIR = lib
SDK_LDDIR	= ld
SDK_INCDIR = include

INC=-I $(TOPDIR)/libesphttpd/espfs -I $(TOPDIR)/libesphttpd/core -I $(TOPDIR)/libesphttpd/util -I . -I $(TOPDIR)/libesphttpd/lib/heatshrink -I $(SDK_BASE)/include

# compiler flags using during compilation of source files
CFLAGS=-Os -ggdb -std=c99 -Werror -Wpointer-arith -Wundef -Wall -Wl,-EL -fno-inline-functions -nostdlib -mlongcalls -mtext-section-literals  -D__ets__ -DICACHE_FLASH -Wno-address -DESPFS_HEATSHRINK -DHTTPD_WEBSOCKETS

# If libtheft is available, build additional property-based tests.
# Uncomment these to use it in test_heatshrink_dynamic.
#CFLAGS += -DHEATSHRINK_HAS_THEFT
#LDFLAGS += -ltheft

# for libesphttpd/espfs/mkespfsimage, built natively
HOSTCFLAGS = -I $(TOPDIR)/libesphttpd/lib/heatshrink -I $(TOPDIR)/libesphttpd/include -I $(TOPDIR)/libesphttpd/espfs -std=gnu99
HOSTCC = gcc

ifeq ("$(GZIP_COMPRESSION)","yes")
CFLAGS += -DGZIP_COMPRESSION -lz
HOSTCFLAGS += -DESPFS_GZIP -lz
endif

ifeq ("$(USE_HEATSHRINK)","yes")
CFLAGS += -DESPFS_HEATSHRINK
HOSTCFLAGS += -DESPFS_HEATSHRINK
endif

ifeq ("$(OUTPUT_TYPE)","ota")
CFLAGS += -DOTA_FLASH_SIZE_K=$(ESP_SPI_FLASH_SIZE_K)
endif

ifeq ("$(OUTPUT_TYPE)","separate")
#In case of separate ESPFS and binaries, set the pos and length of the ESPFS here. 
ESPFS_POS = 0x18000
ESPFS_SIZE = 0x28000
endif

CFLAGS += $(INC)
