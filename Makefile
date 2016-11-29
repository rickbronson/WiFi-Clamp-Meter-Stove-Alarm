# search up in directory tree for  'esphttpdconfig.mk'
TOPDIR ?= $(shell TOPD=$$PWD ; until [ -f esphttpdconfig.mk -o "$$TOPD" = "/" ] ; do cd .. ; TOPD=$$PWD ; done ; echo $$TOPD)

# include common definitions (cross toolchain etc.)
-include $(TOPDIR)/esphttpdconfig.mk

#SPI flash size, in K
ESP_SPI_FLASH_SIZE_K=512
#0: QIO, 1: QOUT, 2: DIO, 3: DOUT
ESP_FLASH_MODE=0
#0: 40MHz, 1: 26MHz, 2: 20MHz, 0xf: 80MHz
ESP_FLASH_FREQ_DIV=0

# Output directors to store intermediate compiled files
# relative to the project directory
BUILD_BASE	= build
FW_BASE = firmware

#Appgen path and name
APPGEN		?= $(SDK_BASE)/tools/gen_appbin.py

# name for the target project
TARGET = httpd

# which modules (subdirectories) of the project to include in compiling
MODULES = user
EXTRA_INCDIR = include libesphttpd/include

# libraries used in this project, mainly provided by the SDK
LIBS = c gcc hal phy pp net80211 wpa main lwip esphttpd

CFLAGS += -D_STDINT_H

# linker flags used to generate the main object file
LDFLAGS		= -nostdlib -Wl,--no-check-sections -u call_user_start -Wl,-static

#Additional (maybe generated) ld scripts to link in
# Extra script to tell the linker the correct irom0 memory available
# EXTRA_LD_SCRIPTS = ldscript_memspecific.ld

SRC_DIR		:= $(MODULES)
BUILD_DIR	:= $(addprefix $(BUILD_BASE)/,$(MODULES))

SDK_LIBDIR	:= $(addprefix $(SDK_BASE)/,$(SDK_LIBDIR))
SDK_INCDIR	:= $(addprefix -I$(SDK_BASE)/,$(SDK_INCDIR))

SRC		:= $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.c))
ASMSRC		= $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.S))
OBJ		= $(patsubst %.c,$(BUILD_BASE)/%.o,$(SRC))
OBJ		+= $(patsubst %.S,$(BUILD_BASE)/%.o,$(ASMSRC))
APP_AR		:= $(addprefix $(BUILD_BASE)/,$(TARGET)_app.a)


V ?= $(VERBOSE)
ifeq ("$(V)","1")
Q :=
vecho := @true
else
Q := @
vecho := @echo
endif

ifeq ("$(ESPFS_POS)","")
#No hardcoded espfs position: link it in with the binaries.
LIBS += webpages-espfs
else
#Hardcoded espfs location: Pass espfs position to rest of code
CFLAGS += -DESPFS_POS=$(ESPFS_POS) -DESPFS_SIZE=$(ESPFS_SIZE)
endif

#Define default target. If not defined here the one in the included Makefile is used as the default one.
default-tgt: all

#Makefile with the options specific to the build of a non-upgradable firmware with
#the espfs included in the firmware binary.

# linker script used for the linker step
LD_SCRIPT	= eagle.app.v6.ld

.PHONY: ldscript_memspecific.ld

ldscript_memspecific.ld:
	$(vecho) "GEN $@"
	$(Q) echo "MEMORY { irom0_0_seg : org = 0x40240000, len = "$$(printf "0x%X" $$(($(ESP_SPI_FLASH_SIZE_K) * 1024 - 0x4000)))" }"> ldscript_memspecific.ld

# ota: +++++++++++++++++++++
ifeq ("$(OUTPUT_TYPE)","ota")
#Makefile with the options specific to the build of a non-upgradable firmware with
#the espfs combined into the flash binary.

# linker script used for the linker step
LD_SCRIPT_USR1	:= eagle.app.v6.new.$(ESP_SPI_FLASH_SIZE_K).app1.ld
LD_SCRIPT_USR2	:= eagle.app.v6.new.$(ESP_SPI_FLASH_SIZE_K).app2.ld

TARGET_OUT_USR1 := $(addprefix $(BUILD_BASE)/,$(TARGET).user1.out)
TARGET_OUT_USR2 := $(addprefix $(BUILD_BASE)/,$(TARGET).user2.out)
TARGET_OUT	:=  $(TARGET_OUT_USR1) $(TARGET_OUT_USR2)

TARGET_BIN_USR1 := $(addprefix $(BUILD_BASE)/,$(TARGET).user1.bin)
TARGET_BIN_USR2 := $(addprefix $(BUILD_BASE)/,$(TARGET).user2.bin)
TARGET_BIN	:=  $(TARGET_BIN_USR1) $(TARGET_BIN_USR2)
TARGET_OTAFILE := $(addprefix $(BUILD_BASE)/,$(TARGET).ota)

#Convert SPI size into arg for appgen. Format: no=size
FLASH_MAP_CONV:=0:512 2:1024 5:2048 6:4096
ESP_FLASH_SIZE_IX:=$(patsubst %:$(ESP_SPI_FLASH_SIZE_K),%,$(filter %:$(ESP_SPI_FLASH_SIZE_K),$(FLASH_MAP_CONV)))

define genappbin
$(1): $$(APP_AR)
	$$(vecho) LD $$@
	$$(Q) $$(LD) -Llibesphttpd -L$$(SDK_LIBDIR) $(2) $$(LDFLAGS) -Wl,--start-group $$(LIBS) $$(APP_AR) -Wl,--end-group -o $$@

$(3): $(1)
	$$(vecho) APPGEN $$@
	$$(Q) $$(OBJCOPY) --only-section .text -O binary $1 build/eagle.app.v6.text.bin
	$$(Q) $$(OBJCOPY) --only-section .data -O binary $1 build/eagle.app.v6.data.bin
	$$(Q) $$(OBJCOPY) --only-section .rodata -O binary $1 build/eagle.app.v6.rodata.bin
	$$(Q) $$(OBJCOPY) --only-section .irom0.text -O binary $1 build/eagle.app.v6.irom0text.bin
	$$(Q) cd build; COMPILE=gcc PATH=$$(XTENSA_TOOLS_ROOT):$$(PATH) python $$(APPGEN) $(1:build/%=%) 2 $$(ESP_FLASH_MODE) $$(ESP_FLASH_FREQ_DIV) $$(ESP_FLASH_SIZE_IX)
	$$(Q) rm -f eagle.app.v6.*.bin
	$$(Q) mv build/eagle.app.flash.bin $$@
	@echo "** user1.bin uses $$$$(stat -c '%s' $$@) bytes of" $$(ESP_FLASH_MAX) "available"
endef

$(eval $(call genappbin,$(TARGET_OUT_USR1),$$(LD_SCRIPT_USR1),$$(TARGET_BIN_USR1)))
$(eval $(call genappbin,$(TARGET_OUT_USR2),$$(LD_SCRIPT_USR2),$$(TARGET_BIN_USR2)))

# Change tagname to some identifier that's unique for your project. 27 chars max.
TAGNAME ?= "generic"

libesphttpd/mkupgimg/mkupgimg: libesphttpd/mkupgimg/
	make -C libesphttpd/mkupgimg/

$(FW_BASE): $(TARGET_BIN) libesphttpd/mkupgimg/mkupgimg
	$(Q) libesphttpd/mkupgimg/mkupgimg $(TARGET_BIN_USR1) $(TARGET_BIN_USR2) $(TAGNAME) $(TARGET_OTAFILE)

flash: $(TARGET_OUT) $(FW_BASE)
	$(Q) $(ESPTOOL) $(ESPTOOL_OPTS) write_flash $(ESPTOOL_FLASHDEF) 0x00000 "$(SDK_BASE)/bin/boot_v1.2.bin" 0x1000 $(TARGET_BIN_USR1)

else

TARGET_OUT	:= $(addprefix $(BUILD_BASE)/,$(TARGET).out)
$(TARGET_OUT): $(APP_AR) $(EXTRA_LD_SCRIPTS)
	$(vecho) "LD $@"
	$(Q) $(LD) -Llibesphttpd -L$(SDK_LIBDIR) $(LD_SCRIPT) $(EXTRA_LD_SCRIPTS) $(LDFLAGS) -Wl,--start-group $(LIBS) $(APP_AR) -Wl,--end-group -o $@ 
	$(OBJDUMP) -S $@ > $(TARGET).lis

$(FW_BASE): $(TARGET_OUT)
	$(vecho) "FW $@"
	$(Q) mkdir -p $@
	$(Q) $(ESPTOOL) elf2image $(TARGET_OUT) --output $@/

#flash: $(TARGET_OUT) $(FW_BASE)
flash:
	$(Q) $(ESPTOOL) $(ESPTOOL_OPTS) write_flash $(ESPTOOL_FLASHDEF) 0x00000 $(FW_BASE)/0x00000.bin 0x40000 $(FW_BASE)/0x40000.bin
	$(Q) $(ESPTOOL) $(ESPTOOL_OPTS) run

endif

# +++++++++++++ end of ota

blankflash:
	$(Q) $(ESPTOOL) $(ESPTOOL_OPTS) write_flash $(ESPTOOL_FLASHDEF) 0x7E000 $(SDK_BASE)/bin/blank.bin 0x7C000 $(SDK_BASE)/bin/esp_init_data_default.bin

htmlflash:
	$(Q) if [ $$(stat -c '%s' libesphttpd/webpages.espfs) -gt $$(( $(ESPFS_SIZE) )) ]; then echo "webpages.espfs too big, is $$(stat -c '%s' libesphttpd/webpages.espfs) max = $(ESPFS_SIZE)!"; false; fi
	$(Q) $(ESPTOOL) $(ESPTOOL_OPTS) write_flash  $(ESPTOOL_FLASHDEF) $(ESPFS_POS) libesphttpd/webpages.espfs
	$(Q) $(ESPTOOL) $(ESPTOOL_OPTS) run

run:
	$(Q) $(ESPTOOL) $(ESPTOOL_OPTS) run

#Add all prefixes to paths
LIBS		:= $(addprefix -l,$(LIBS))
LD_SCRIPT	:= $(addprefix -T$(SDK_BASE)/$(SDK_LDDIR)/,$(LD_SCRIPT))
LD_SCRIPT_USR1	:= $(addprefix -T$(SDK_BASE)/$(SDK_LDDIR)/,$(LD_SCRIPT_USR1))
LD_SCRIPT_USR2	:= $(addprefix -T$(SDK_BASE)/$(SDK_LDDIR)/,$(LD_SCRIPT_USR2))

INCDIR	:= $(addprefix -I,$(SRC_DIR))
EXTRA_INCDIR	:= $(addprefix -I,$(EXTRA_INCDIR))
MODULE_INCDIR	:= $(addsuffix /include,$(INCDIR))

define maplookup
$(patsubst $(strip $(1)):%,%,$(filter $(strip $(1)):%,$(2)))
endef

ESP_FLASH_SIZE_IX=$(call maplookup,$(ESP_SPI_FLASH_SIZE_K),512:0 1024:2 2048:5 4096:6)
ESPTOOL_FREQ=$(call maplookup,$(ESP_FLASH_FREQ_DIV),0:40m 1:26m, 2:20m 0xf:80m)
ESPTOOL_MODE=$(call maplookup,$(ESP_FLASH_MODE),0:qio 1:qout 2:dio 3:dout)
ESPTOOL_SIZE=$(call maplookup,$(ESP_SPI_FLASH_SIZE_K),512:4m 256:2m 1024:8m 2048:16m 4096:32m)

ESPTOOL_OPTS=--port $(ESPPORT) --baud $(ESPBAUD)
ESPTOOL_FLASHDEF=--flash_freq $(ESPTOOL_FREQ) --flash_mode $(ESPTOOL_MODE) --flash_size $(ESPTOOL_SIZE)

vpath %.c $(SRC_DIR)
vpath %.S $(SRC_DIR)

define compile-objects
$1/%.o: %.c
	$(vecho) "CC $$<"
	$(Q) $(CC) $(INCDIR) $(MODULE_INCDIR) $(EXTRA_INCDIR) $(SDK_INCDIR) $(CFLAGS)  -c $$< -o $$@

$1/%.o: %.S
	$(vecho) "CC $$<"
	$(Q) $(CC) $(INCDIR) $(MODULE_INCDIR) $(EXTRA_INCDIR) $(SDK_INCDIR) $(CFLAGS)  -c $$< -o $$@
endef

.PHONY: all checkdirs clean libesphttpd default-tgt

all: checkdirs $(TARGET_OUT) $(FW_BASE)

libesphttpd/Makefile:
	$(Q) echo "No libesphttpd submodule found. Using git to fetch it..."
	$(Q) git submodule init
	$(Q) git submodule update

libesphttpd: libesphttpd/Makefile
	$(Q) make -C libesphttpd

$(APP_AR): libesphttpd $(OBJ)
	$(vecho) "AR $@"
	$(Q) $(AR) cru $@ $(OBJ)

checkdirs: $(BUILD_DIR)

$(BUILD_DIR):
	$(Q) mkdir -p $@

clean:
	$(Q) make -C libesphttpd clean
	$(Q) rm -f $(APP_AR)
	$(Q) rm -f $(TARGET_OUT)
	$(Q) find $(BUILD_BASE) -type f | xargs rm -f
	$(Q) rm -rf $(FW_BASE)

$(foreach bdir,$(BUILD_DIR),$(eval $(call compile-objects,$(bdir))))
