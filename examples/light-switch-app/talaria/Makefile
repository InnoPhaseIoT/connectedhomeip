
FACTORY_DATA_PROVIDER ?= false
DEVICE_INSTANCE_INFO_PROVIDER ?= false

all: out/test/build.ninja out/test/light-switch-app.elf

out/test/light-switch-app.elf: FORCE
	ninja -C out/test -j 4
	arm-none-eabi-objcopy --strip-all out/test/light-switch-app.elf out/test/light-switch-app.elf.strip
	ls -lah out/test/light-switch-app.elf out/test/light-switch-app.elf.strip
	cksum out/test/light-switch-app.elf
	cksum out/test/light-switch-app.elf.strip

out/test/build.ninja:
	gn gen out/test --args="is_debug=false config_enable_talaria_factory_data_provider=$(FACTORY_DATA_PROVIDER) config_enable_talaria_device_instance_info_provider=$(DEVICE_INSTANCE_INFO_PROVIDER)"

clean:
	rm -rf out/

FORCE:
