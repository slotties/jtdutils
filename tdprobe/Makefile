include ../Makefile.inc
include ../generic_tool.mk

TOOL=tdprobe
CUSTOM_LDFLAGS=-ldl -lpthread -export-dynamic

compile: compile_modules

clean: clean_modules

compile_modules:
	make -C modules clean all

clean_modules:
	make -C modules clean
