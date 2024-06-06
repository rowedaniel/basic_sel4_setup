# Modified from sDDF's example Makefile
# 
# Copyright 2022, UNSW
#
# SPDX-License-Identifier: BSD-2-Clause
#

BOARD ?= odroidc4
MICROKIT_ROOT ?= /microkit
SDDF_ROOT ?= sDDF

BUILD := build


# Resources
export BOARD := $(BOARD)
BUILD := $(abspath $(BUILD))
export MICROKIT_ROOT := $(abspath $(MICROKIT_ROOT))
export SDDF := $(abspath $(SDDF_ROOT))
export SRC := $(abspath ./)

# Cross compiler
TRIPLE := aarch64-none-elf

# Tools
export CC := $(TRIPLE)-gcc
export LD := $(TRIPLE)-ld
export AR := $(TRIPLE)-ar
export AS := $(TRIPLE)-as
export RANLIB := $(TRIPLE)-ranlib
export MICROKIT_TOOL := $(MICROKIT_ROOT)/bin/microkit

export IMAGE_FILE := $(BUILD)/loader.img
export REPORT_FILE := $(BUILD)/report.txt

all: $(IMAGE_FILE)
	
$(IMAGE_FILE) $(REPORT_FILE) clean clobber: $(BUILD)/Makefile
	$(MAKE) -C $(BUILD) MICROKIT_ROOT=$(MICROKIT_ROOT) $@

$(BUILD)/Makefile: tx_rx.mk
	mkdir -p $(BUILD)
	cp tx_rx.mk $@

