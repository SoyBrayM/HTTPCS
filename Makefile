all: out
	./out

out: main.c
	clang -O3 -Wall main.c -o out
