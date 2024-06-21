# Directory in which the build artifacts are collected
O := board/$(BOARD)


CONFIG ?= debug
BOARD_PATH := $(MICROKIT_ROOT)/board/$(BOARD)/$(CONFIG)
LIBC := $(dir $(realpath $(shell aarch64-none-elf-gcc --print-file-name libc.a)))
SYSTEM_FILE := $(SRC)/board/$(BOARD)/tx_rx.system


# sDDF
SDDF_INC := $(SDDF)/include
SDDF_UTIL := $(SDDF)/util
SDDF_CLOCK := $(SDDF)/drivers/clock/$(DRIV_DIR)

ETHERNET_DRIVER:=$(SDDF)/drivers/network/$(DRIV_DIR)
ETHERNET_CONFIG_INCLUDE:=$(SRC)/include/ethernet_config

vpath %.c $(SDDF) $(SRC)

# Flags
CPU := cortex-a55
CFLAGS := \
		-mcpu=$(CPU) \
		-mstrict-align \
		-ffreestanding \
		-g3 -O3 -Wall \
		-Wno-unused-function \
		-DMICROKIT_CONFIG_$(CONFIG) \
		-I$(BOARD_PATH)/include \
		-I$(SDDF_INC) \
		-I$(ETHERNET_CONFIG_INCLUDE) \
		-MD \
		-MP
LDFLAGS += -L$(BOARD_PATH)/lib -L$(LIBC)
LIBS := --start-group -lmicrokit -Tmicrokit.ld -lc libsddf_util_debug.a --end-group

# Generic rules for object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

timer.o: $(SDDF_CLOCK)/timer.c
	$(CC) $(CFLAGS) -c $< -o $@

board_src/%.o: board/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Create the build directory
$O:
	@mkdir -p $O $O/src $O/test $O/generated $O/board/$(BOARD) $O/sddf

# elfs
# tx.elf: tx.o libsddf_util.a libsddf_util_debug.a
# 	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

rx.elf: rx.o libsddf_util.a libsddf_util_debug.a
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@
	
# timer.elf: timer.o
# 	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

eth_driver.elf: libsddf_util_debug.a
	
# Bootable
# $(IMAGE_FILE) $(REPORT_FILE): $O tx.elf rx.elf timer.elf $(SYSTEM_FILE)
$(IMAGE_FILE) $(REPORT_FILE): $O rx.elf eth_driver.elf $(SYSTEM_FILE)
	$(MICROKIT_TOOL) $(SYSTEM_FILE) --search-path $O --board $(BOARD) --config $(CONFIG) -o $(IMAGE_FILE) -r $(REPORT_FILE)
	
	
include $(SDDF_UTIL)/util.mk
include $(SDDF_CLOCK)/timer_driver.mk
include $(ETHERNET_DRIVER)/eth_driver.mk
	
-include tx.d rx.d timer.d