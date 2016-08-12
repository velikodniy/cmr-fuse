CFLAGS=-g -Wall $(shell pkg-config --cflags libcurl jansson)
LDFLAGS=$(shell pkg-config --libs libcurl jansson)
CC=clang

cmr-fuse: cmr-fuse.c
