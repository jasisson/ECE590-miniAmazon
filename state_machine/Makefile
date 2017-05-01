CC=g++
CFLAGS=-std=c++11 -g
EXTRAFLAGS=-lpqxx -lpq

all: state

state: main.cpp order.cpp StateMachine.cpp
	$(CC) $(CFLAGS) -o state main.cpp order.cpp StateMachine.cpp

clean:
	rm -rf *~ *.o *.dSYM state

clobber:
	rm -f *~ *.o
