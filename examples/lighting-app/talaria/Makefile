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
TARGET := $(OUT_DIR)/lighting-app.elf
TARGET_STRIP := $(TARGET).strip
TARGET_SIGN := $(TARGET).sign
TARGET_ENC := $(TARGET).enc

ENCRYPT_SECTIONS := text data
ENCRYPT_FILES := $(foreach sect,$(ENCRYPT_SECTIONS),$(sect).bin $(sect).enc.bin)

CFLAGS += -I$(COMPONENTS_DIR)

# Default values if not defined
SECURED ?= false
EXT_FLASH ?= false
ENABLE_HTTP_APP ?= false
ENABLE_MQTT_APP ?= false
ENABLE_AWS_IOT_APP ?= false
ENABLE_OTA_REQUESTOR ?= true

COMPILE_AWS_IOT_SDK_LIBS := talaria_aws_iot/compile_aws_sdk.sh

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
	 @if [ "$(ENABLE_AWS_IOT_APP)" = "true" ]; then \
                echo "Executing $(COMPILE_AWS_IOT_SDK_LIBS)"; \
                $(COMPILE_AWS_IOT_SDK_LIBS); \
                if [ $$? -ne 0 ]; then \
                        exit 1; \
                fi; \
        fi
	ninja -C $(OUT_DIR) -j 4

$(OUT_DIR)/build.ninja:
	gn gen $(OUT_DIR) --args="is_debug=false chip_enable_secureboot=$(SECURED) chip_enable_ext_flash=$(EXT_FLASH) config_enable_http_client_app=$(ENABLE_HTTP_APP) config_enable_mqtt_app=$(ENABLE_MQTT_APP) config_enable_aws_iot_app=$(ENABLE_AWS_IOT_APP) chip_enable_ota_requestor=$(ENABLE_OTA_REQUESTOR)"

ifdef KEY
	$(info KEY=$(KEY))
endif

ifdef SECURED
	$(info SECUREBOOT=$(SECURED))
endif

ifdef EXT_FLASH
	$(info EXTERNAL_FLASH=$(EXT_FLASH))
endif

clean:
	rm -rf $(OUT_DIR)

.PHONY: all clean check_key FORCE
