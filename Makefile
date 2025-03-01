all: run

compile:
	gcc -o main main.c -lncurses

run: compile
	./main