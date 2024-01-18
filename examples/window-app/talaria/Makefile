
all: out/test/build.ninja out/test/window-app.elf

out/test/window-app.elf: FORCE
	ninja -C out/test -j 4
	ls -lah out/test/window-app.elf
	cksum out/test/window-app.elf

out/test/build.ninja:
	gn gen out/test --args="is_debug=false"

clean:
	rm -rf out/

FORCE:
