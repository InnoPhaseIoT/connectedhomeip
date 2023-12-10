
all: out/test/build.ninja out/chip-example-app.elf

out/chip-example-app.elf: FORCE
	ninja -C out/test/
	./4_gn_link.sh
	ls -lah out/chip-example-app.elf
	cksum out/chip-example-app.elf

out/test/build.ninja:
	gn gen out/test --args="is_debug=false"

clean:
	rm -rf out/

FORCE:
