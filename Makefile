CFLAGS = -Wall -Wno-logical-op-parentheses -O3

main:
	gcc $(CFLAGS) main.c src/*.c src/*/*.c -o editor
