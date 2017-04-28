all: runAmazon

runAmazon: connection.cpp
	g++ -o runAmazon connection.cpp -lprotobuf -std=c++11
