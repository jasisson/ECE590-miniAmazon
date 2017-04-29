all: runAmazon dbsetup

runAmazon: connection.cpp
	g++ -o runAmazon connection.cpp -lprotobuf -std=c++11

dbsetup: db.cpp main.cpp db.hpp
	g++ -o dbsetup main.cpp db.cpp -lpqxx -lpq -std=c++11

clean:
	rm *~ *# runAmazon
