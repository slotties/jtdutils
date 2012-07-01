include Makefile.inc

TOOLS=tdstrip tdgrep tdfilter tdstat tdlocks tdls tdpager tdprobe tuid

all: $(TOOLS)
	
clean:
	rm -rf $(FULL_NAME)
	
	make -C libtd clean
	for t in $(TOOLS); do\
		make -C $$t TOOL=$$t BUILD_ENV=$(BUILD_ENV) clean \
	;done

man:
	for t in $(TOOLS); do\
		make -C $$t TOOL=$$t BUILD_ENV=$(BUILD_ENV) man \
	;done

install: $(TOOLS) 
# FIXME: zshcomp
	for t in $(TOOLS); do\
		make -C $$t TOOL=$$t BUILD_ENV=$(BUILD_ENV) install \
	;done

libtd:
	make -C ./libtd

$(TOOLS): libtd
	make -C $@ BUILD_ENV=$(BUILD_ENV) compile

unit_tests: libtd
	make -C test

# Special tools
zshcomp:
	cp tools/zsh/_jtdutils $(ZSH_FUNC_DIR)
	chmod 644 $(ZSH_FUNC_DIR)/_jtdutils

# Distributions
dist-src: clean
	mkdir -p $(FULL_NAME)

	# copy all files
	cp Makefile Makefile.inc generic_tool.mk $(FULL_NAME)/
	cp -R $(TOOLS) libtd $(FULL_NAME)/
	cp CHANGELOG ROADMAP GPL-3.0 $(FULL_NAME)/

	# remove unnecessary stuff
	find $(FULL_NAME) -name ".svn" | xargs rm -rf

	# build package
	tar -cf $(FULL_NAME)-src.tar $(FULL_NAME)
	gzip -f $(FULL_NAME)-src.tar
	
dist-bin: clean all

arch: dist-src 
	# update md5sum
	sed -i 's/md5sum.*/'`makepkg -g`'/g' PKGBUILD
	makepkg -f
	# cleanup
	rm -rf src pkg
	
.PHONY: all clean install zshcomp unit_tests libtd $(TOOLS)
