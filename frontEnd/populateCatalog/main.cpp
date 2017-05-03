#include <iostream>
#include <fstream>
#include <sstream>
#include <pqxx/pqxx>
#include <string>

#include "add_cat.h"

using namespace std;
using namespace pqxx;

void parseCatalog (connection *C) {
  string line, attr;
  int count = 0;
  vector<string> v;
  ifstream catalogFile ("catalog.txt");
  if (catalogFile.is_open()) {
    while (getline (catalogFile, line)) {
      if (line.empty()) {
	continue;}
      stringstream ss(line);
      while (getline(ss, attr, '%')) {
	v.push_back(attr);
      }
      count++;
    }
  }
  else {
    cout << "Unable to open file." << endl;
  }
  int i = 0;
  for(i; i < count; i++) {
    int set = i * 3;
    string desc = v[set];
    int pid = stoi(v[set+1]);
    string kind = v[set+2];
    add_catalog(C, desc, pid, kind);
   }
  catalogFile.close();
}

int main (int argc, char *argv[]) {
  //Allocate & initialize a Postgres connection object
  connection *C;

  string cString;

  try{
    //Establish a connection to the database
    //Parameters: database name, user name, user password
    C = new connection("dbname=polls user=jalon password=jalon");
    if (C->is_open()) {
      cout << "Opened database successfully: " << C->dbname() << endl;
    } else {
      cout << "Can't open database" << endl;
      return 1;
    }
  } catch (const std::exception &e){
    cerr << e.what() << std::endl;
    return 1;
  }

  //Parse .txt files and insert
  parseCatalog(C);
  //exercise(C);
  
 
  //Close database connection
  C->disconnect();

  return 0;
}


