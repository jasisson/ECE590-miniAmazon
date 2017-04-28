CC=g++
CFLAGS=-std=c++11 -g
EXTRAFLAGS=-lpqxx -lpq

all: amazon

amazon: main.cpp db.cpp db.hpp
	$(CC) $(CFLAGS) -o amazon main.cpp db.cpp $(EXTRAFLAGS)

clean:
	rm -f *~ *.o amazon

clobber:
	rm -f *~ *.o
