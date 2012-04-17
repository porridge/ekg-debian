all: parse

CFLAGS ?= -O2 -s

parse: parse.c
	gcc $(CFLAGS) -o parse parse.c  `pkg-config --cflags --libs glib-2.0` # -ggdb

clean:
	rm -f parse
