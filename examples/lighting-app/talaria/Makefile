
all: out/test/build.ninja out/test/lighting-app.elf

out/test/lighting-app.elf: FORCE
	ninja -C out/test -j 4
	ls -lah out/test/lighting-app.elf
	cksum out/test/lighting-app.elf

out/test/build.ninja:
	gn gen out/test --args="is_debug=false"

clean:
	rm -rf out/

FORCE:
