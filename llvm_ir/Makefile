all: ir gen

.PHONY: ir gen

ir:
	llc test.ll -relocation-model=pic
	g++ test.s -o test


FLAGS = $(shell llvm-config --cflags --ldflags --libs)

# to execute llvm ir: ./gen | lli
gen:
	g++ -Wall -Wextra -g main.cpp -o gen $(FLAGS)
