
all: out/test/build.ninja out/test/basic-video-player-app.elf

out/test/basic-video-player-app.elf: FORCE
	ninja -C out/test -j 4
	arm-none-eabi-objcopy --strip-all out/test/basic-video-player-app.elf out/test/basic-video-player-app.elf.strip
	ls -lah out/test/basic-video-player-app.elf out/test/basic-video-player-app.elf.strip
	cksum out/test/basic-video-player-app.elf
	cksum out/test/basic-video-player-app.elf.strip

out/test/build.ninja:
	gn gen out/test --args="is_debug=false"

clean:
	rm -rf out/

FORCE:
