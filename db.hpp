#include <iostream>
#include <pqxx/pqxx>
#include <string>

using namespace std;
using namespace pqxx;

#ifndef _SETUPDB_
#define _SETUPDB_


void sell_items (connection *C, string itemDesc, unsigned int toSell);

void add_items(connection *C, string itemDesc, unsigned int itemQuant);

void parseInventory (connection *C);

connection * setupDB (int reset);


#endif //_SETUPDB_
