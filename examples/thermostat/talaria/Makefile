
all: out/test/build.ninja out/test/thermostat-app.elf

out/test/thermostat-app.elf: FORCE
	ninja -C out/test -j 4
	arm-none-eabi-objcopy --strip-all out/test/thermostat-app.elf out/test/thermostat-app.elf.strip
	ls -lah out/test/thermostat-app.elf out/test/thermostat-app.elf.strip
	cksum out/test/thermostat-app.elf
	cksum out/test/thermostat-app.elf

out/test/build.ninja:
	gn gen out/test --args="is_debug=false"

clean:
	rm -rf out/

FORCE:
