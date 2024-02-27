main:
	gcc main.c src/*.c

other:
	gcc -m32 main.c src/*.c
