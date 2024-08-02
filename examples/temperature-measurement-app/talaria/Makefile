
all: out/test/build.ninja out/temperature-measurement-app.elf

out/temperature-measurement-app.elf: FORCE
	ninja -C out/test/ -j 4
	arm-none-eabi-objcopy --strip-all out/test/temperature-measurement-app.elf out/test/temperature-measurement-app.elf.strip
	ls -lah out/test/temperature-measurement-app.elf out/test/temperature-measurement-app.elf.strip
	cksum out/test/temperature-measurement-app.elf
	cksum out/test/temperature-measurement-app.elf.strip

out/test/build.ninja:
	gn gen out/test --args="is_debug=false"

clean:
	rm -rf out/

FORCE:
