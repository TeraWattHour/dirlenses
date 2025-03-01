all: run

compile:
	gcc -o dirlenses main.c -lncurses

run: compile
	./dirlenses