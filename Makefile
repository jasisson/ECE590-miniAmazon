all: runAmazon

runAmazon: connection.cpp
	g++ -o runAmazon connection.cpp -std=c++11

clean:
	rm *~ 
