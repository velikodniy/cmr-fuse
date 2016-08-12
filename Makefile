CFLAGS=-g -Wall $(shell pkg-config --cflags libcurl jansson fuse) -D_FILE_OFFSET_BITS=64
LDFLAGS=$(shell pkg-config --libs libcurl jansson fuse)
CC=clang

cmr-fuse: cmr-fuse.c cmrapi.c list.c
