all: run

compile:
	gcc -o dirlenses main.c finfo.c -lncurses

run: compile
	./dirlenses