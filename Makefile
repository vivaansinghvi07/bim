main:
	gcc main.c -O3 src/*.c src/*/*.c src/*/*/*.c -o editor -Wall -Wno-logical-op-parentheses
