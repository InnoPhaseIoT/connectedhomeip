
all: out/test/build.ninja out/test/lock-app.elf

out/test/lock-app.elf: FORCE
	ninja -C out/test -j 4
	arm-none-eabi-objcopy --strip-all out/test/lock-app.elf out/test/lock-app.elf.strip
	ls -lah out/test/lock-app.elf out/test/lock-app.elf.strip
	cksum out/test/lock-app.elf
	cksum out/test/lock-app.elf.strip

out/test/build.ninja:
	gn gen out/test --args="is_debug=false"

clean:
	rm -rf out/

FORCE:
