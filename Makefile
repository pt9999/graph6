all: graph6

graph6: graph6.cc
	g++ -Wall -O2 graph6.cc -o graph6

run: graph6
	./graph6 | less
