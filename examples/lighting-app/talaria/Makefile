TOP = ../../../third_party/talaria/repo/freertos_sdk
TOP := $(abspath $(TOP))
-include $(TOP)/embedded_apps.mak
SDK_DIR ?= $(TOP)
ROOT_DIR := $(SDK_DIR)

# Basic rules and variables
include $(ROOT_DIR)/build/rules.mak
include $(TOP)/build.mak

# Variables
OUT_DIR := out/test
TARGET := $(OUT_DIR)/on-off-light-app.elf
TARGET_STRIP := $(TARGET).strip
TARGET_SIGN := $(TARGET).sign
TARGET_ENC := $(TARGET).enc

ENCRYPT_SECTIONS := text data
ENCRYPT_FILES := $(foreach sect,$(ENCRYPT_SECTIONS),$(sect).bin $(sect).enc.bin)

CFLAGS += -I$(COMPONENTS_DIR)

# Targets
all: check_key libcomponents $(TARGET)

$(TARGET): $(TARGET_STRIP)
	ninja -C $(OUT_DIR) -j 4
	arm-none-eabi-objcopy --strip-all $(TARGET) $(TARGET_STRIP)
ifdef KEY
	$(ROOT_DIR)/script/signature.py --output signature.bin --key=$(KEY) $<
	arm-none-eabi-objcopy --add-section .signature=signature.bin $(TARGET_STRIP) $(TARGET_SIGN)
	rm -f signature.bin
	arm-none-eabi-objcopy $(foreach sect,$(ENCRYPT_SECTIONS),--dump-section .$(sect)=$(sect).bin) $(TARGET_SIGN) $(TARGET_ENC)
	@for sect in $(ENCRYPT_SECTIONS); do \
		if [ -s "$${sect}.bin" ]; then \
			addr=$$($(SDK_DIR)/script/print_section_addr.py $(TARGET_SIGN) ".$$sect") && \
			echo "Encrypting .$$sect @ 0x$${addr}..." && \
			$(SDK_DIR)/script/xex.py --keyfile=$(KEY) --init "$$addr" "$${sect}.bin" "$${sect}.enc.bin" && \
			arm-none-eabi-objcopy --update-section ".$$sect"="$${sect}.enc.bin" $@ \
			|| exit $$?; \
		fi; \
	done
	rm -f $(ENCRYPT_FILES)
endif

$(TARGET_STRIP): $(OUT_DIR)/build.ninja FORCE
	ninja -C $(OUT_DIR) -j 4

$(OUT_DIR)/build.ninja:
ifdef SECURED
	gn gen $(OUT_DIR) --args="is_debug=false chip_enable_secureboot=$(SECURED)"
else
	gn gen $(OUT_DIR) --args="is_debug=false chip_enable_secureboot=false"
endif

ifdef KEY
	$(info KEY=$(KEY))
endif

ifdef SECURED
	$(info SECUREBOOT=$(SECURED))
endif

clean:
	rm -rf $(OUT_DIR)

.PHONY: all clean check_key FORCE
