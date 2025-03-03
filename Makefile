CMD = gcc -o dirlenses main.c finfo.c cache.c dir.c -lncurses

all: run

compile:
	$(CMD)

run: compile
	./dirlenses

debug:
	$(CMD) -g -DDEBUG

clean:
	rm -rf dirlenses dirlenses.dSYM