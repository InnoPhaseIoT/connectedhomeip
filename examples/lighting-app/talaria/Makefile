
all: out/test/build.ninja out/test/lighting-app.elf

out/test/lighting-app.elf: FORCE
	ninja -C out/test -j 4
	arm-none-eabi-objcopy --strip-all out/test/lighting-app.elf out/test/lighting-app.elf.strip
	ls -lah out/test/lighting-app.elf out/test/lighting-app.elf.strip
	cksum out/test/lighting-app.elf
	cksum out/test/lighting-app.elf.strip

out/test/build.ninja:
	gn gen out/test --args="is_debug=false"

clean:
	rm -rf out/

FORCE:
