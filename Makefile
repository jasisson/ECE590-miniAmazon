all: runAmazon dbsetup runUPS

runAmazon: connection.cpp 
	g++ -o runAmazon connection.cpp -lpqxx -lpq -lprotobuf -std=c++11

dbsetup: db.cpp main.cpp db.hpp
	g++ -o dbsetup main.cpp db.cpp -lpqxx -lpq -std=c++11

runUPS: upsconnection.cpp
	g++ -o runUPS upsconnection.cpp -lpqxx -lpq -lprotobuf -std=c++11

clean:
	rm -rf *~ *# runAmazon
