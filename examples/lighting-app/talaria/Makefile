
all: out/test/build.ninja out/test/on-off-light-app.elf

out/test/on-off-light-app.elf: FORCE
	ninja -C out/test -j 4
	arm-none-eabi-objcopy --strip-all out/test/on-off-light-app.elf out/test/on-off-light-app.elf.strip
	ls -lah out/test/on-off-light-app.elf out/test/on-off-light-app.elf.strip
	cksum out/test/on-off-light-app.elf
	cksum out/test/on-off-light-app.elf.strip

out/test/build.ninja:
	gn gen out/test --args="is_debug=false"

clean:
	rm -rf out/

FORCE:
