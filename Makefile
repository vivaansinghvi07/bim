CFLAGS = -Wall -Wno-logical-op-parentheses -O3
FILES = main.c src/*.c src/*/*.c

compile:
	gcc $(CFLAGS) $(FILES) -o editor

install: compile
	cp ./editor /usr/local/bin/

test: 
	gcc $(CFLAGS) $(FILES) -o test_editor
