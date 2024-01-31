
all: out/test/build.ninja out/test/Talaria-Application

out/test/Talaria-Application: FORCE
	ninja -C out/test -j 4
	ls -lah out/test/Talaria-Application
	cksum out/test/Talaria-Application

out/test/build.ninja:
	gn gen out/test --args="is_debug=false"

clean:
	rm -rf out/

FORCE:
