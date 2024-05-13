CFLAGS = -Wall -Wno-logical-op-parentheses -O3
FILES = main.c src/*.c src/*/*.c
NAME = bim
TEST_NAME = bim_test

compile:
	gcc $(CFLAGS) $(FILES) -o $(NAME)

install: compile
	cp ./$(NAME) /usr/local/bin/

test: 
	gcc $(CFLAGS) $(FILES) -o $(TEST_NAME)

clean:
	rm -f /usr/local/bin/$(NAME)
	rm -f ./$(TEST_NAME)
	rm -f ./$(NAME)
