
all: out/test/build.ninja out/test/smoke-co-alarm-app.elf

out/test/smoke-co-alarm-app.elf: FORCE
	ninja -C out/test -j 4
	arm-none-eabi-objcopy --strip-all out/test/smoke-co-alarm-app.elf out/test/smoke-co-alarm-app.elf.strip
	ls -lah out/test/smoke-co-alarm-app.elf out/test/smoke-co-alarm-app.elf.strip
	cksum out/test/smoke-co-alarm-app.elf
	cksum out/test/smoke-co-alarm-app.elf.strip

out/test/build.ninja:
	gn gen out/test --args="is_debug=false"

clean:
	rm -rf out/

FORCE:
