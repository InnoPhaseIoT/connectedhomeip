
EXT_FLASH ?= false
SECURED ?= false

all: out/test/build.ninja out/test/window-app.elf

out/test/window-app.elf: FORCE
	ninja -C out/test -j 4
	arm-none-eabi-objcopy --strip-all out/test/window-app.elf out/test/window-app.elf.strip
	ls -lah out/test/window-app.elf out/test/window-app.elf.strip
	cksum out/test/window-app.elf
	cksum out/test/window-app.elf.strip

out/test/build.ninja:
	gn gen out/test --args="is_debug=false chip_enable_ext_flash=$(EXT_FLASH) chip_enable_secureboot=$(SECURED)"

clean:
	rm -rf out/

FORCE:
