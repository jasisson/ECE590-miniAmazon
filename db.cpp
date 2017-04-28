#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "db.hpp"

using namespace std;
using namespace pqxx;

void sell_items(connection *C, string itemDesc, unsigned int toSell) {

}


void add_items(connection *C, string itemDesc, unsigned int itemQuant) {
  string sql;

  cout << "desc:" << itemDesc << endl;
  cout << "quant" << itemQuant << endl;

  
  sql = "INSERT INTO INVENTORY (ITEM_DESC,ITEM_QUANT)" \
    "VALUES ('" + itemDesc + "'," + to_string(itemQuant) + ");";

   /* Create a transactional object. */
   work W(*C);

   /* Execute SQL query */
   W.exec( sql );
   W.commit();
}


void parseInventory (connection *C) {
  string line, attr;
  int count = 0;
  vector<string> v;
  ifstream inventoryFile ("inventory.txt");
  if (inventoryFile.is_open()) {
    while (getline (inventoryFile, line)) {
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
    int set = i * 2;
    //cout << "set+1:" << v[set] << endl;
    string itemDesc = v[set];
    //cout << "set+2:" << v[set+1] << endl;
    unsigned int itemQuant = stoul(v[set+1]);
    add_items(C, itemDesc, itemQuant);
  }
  inventoryFile.close();
}


connection * setupDB (int reset) {
  connection *C;
  string aString;
  string iString;
  
  try{
    C = new connection("dbname=amazon user=postgres password=passw0rd");
    if (C->is_open()) {
      cout << "Opened database successfully: " << C->dbname() << endl;
    } else {
      cout << "Can't open database" << endl;
      //return 1;
    }
  } catch (const std::exception &e){
    cerr << e.what() << std::endl;
    //return 1;
  }
  
  //DROP DB Tables IF reset=true
  if (reset == 1) {
  
  work aD(*C);
  aString = "DROP TABLE IF EXISTS inventory"; 
  aD.exec(aString);
  aD.commit();

  cout << "Table successfully dropped." << endl;
  }
  
  
  // Create Tables
  iString = "CREATE TABLE INVENTORY("	       \
    "PID SERIAL   PRIMARY KEY  NOT NULL,"      \
    "ITEM_DESC    TEXT         NOT NULL,"      \
    "ITEM_QUANT   BIGINT       NOT NULL);";    \

  cout << "Created table INVENTORY successfully." << endl;
  
  //Create transactional objects
  work iW(*C);
  iW.exec( iString );
  iW.commit();

  return C;
}
