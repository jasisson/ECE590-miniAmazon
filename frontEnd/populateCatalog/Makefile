CC=g++
CFLAGS=-std=c++11 -g
EXTRAFLAGS=-lpqxx -lpq

all: test

test: main.cpp add_cat.h add_cat.cpp
	$(CC) $(CFLAGS) -o test main.cpp add_cat.cpp $(EXTRAFLAGS)

clean:
	rm -f *~ *.o test

clobber:
	rm -f *~ *.o
