# jtdutils version
VERSION=0.6
FULL_NAME=jtdutils-$(VERSION)

# Compiler flags
CC=gcc
CFLAGS=-Wall -std=c99 -D_GNU_SOURCE 

ifeq ($(BUILD_ENV),release)
	CFLAGS += -O3 
else
	CFLAGS += -g
endif 

ZSH_VER=4.3.10
ZSH_FUNC_DIR=/usr/share/zsh/$(ZSH_VER)/functions/Completion/Unix

TMP_DIR=target

ifndef BINPATH
	BINPATH=/usr/bin
endif

ifndef MANPATH
	MANPATH=/usr/share/man/man1
endif
