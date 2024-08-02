
all: out/test/build.ninja out/test/air-quality-sensor-app.elf

out/test/air-quality-sensor-app.elf: FORCE
	ninja -C out/test -j 4
	arm-none-eabi-objcopy --strip-all out/test/air-quality-sensor-app.elf out/test/air-quality-sensor-app.elf.strip
	ls -lah out/test/air-quality-sensor-app.elf out/test/air-quality-sensor-app.elf.strip
	cksum out/test/air-quality-sensor-app.elf
	cksum out/test/air-quality-sensor-app.elf.strip

out/test/build.ninja:
	gn gen out/test --args="is_debug=false"

clean:
	rm -rf out/

FORCE:
