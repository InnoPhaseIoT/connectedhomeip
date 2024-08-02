
all: out/test/build.ninja out/contact-sensor-app.elf

out/contact-sensor-app.elf: FORCE
	ninja -C out/test/ -j 4
	arm-none-eabi-objcopy --strip-all out/test/contact-sensor-app.elf out/test/contact-sensor-app.elf.strip
	ls -lah out/test/contact-sensor-app.elf out/test/contact-sensor-app.elf.strip
	cksum out/test/contact-sensor-app.elf
	cksum out/test/contact-sensor-app.elf.strip

out/test/build.ninja:
	gn gen out/test --args="is_debug=false"

clean:
	rm -rf out/

FORCE:
