
all: out/test/build.ninja out/test/speaker-app.elf

out/test/speaker-app.elf: FORCE
	ninja -C out/test -j 4
	arm-none-eabi-objcopy --strip-all out/test/speaker-app.elf out/test/speaker-app.elf.strip
	ls -lah out/test/speaker-app.elf out/test/speaker-app.elf.strip
	cksum out/test/speaker-app.elf
	cksum out/test/speaker-app.elf.strip

out/test/build.ninja:
	gn gen out/test --args="is_debug=false"

clean:
	rm -rf out/

FORCE:
