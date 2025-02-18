run:
	@clang++ -std=c++20 -Wall -Wextra -o server.out main.cc
	@./server.out

build:
	@clang++ -std=c++20 -Wall -Wextra -o server.out main.cc