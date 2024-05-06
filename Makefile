CFLAGS = -Wall -Wno-logical-op-parentheses -O3

compile:
	gcc $(CFLAGS) main.c src/*.c src/*/*.c -o editor

install: compile
	cp ./editor /usr/local/bin/
