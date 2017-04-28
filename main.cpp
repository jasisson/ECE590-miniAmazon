#include <iostream>
#include <fstream>
#include <sstream>
#include <pqxx/pqxx>
#include <string>
#include "db.hpp"

using namespace std;
using namespace pqxx;

int main (int argc, char *argv[]) {

  //Set up socket to website. Init DB
  connection *C;
  C = setupDB(1); //If int == 1, reset DB
  parseInventory(C);
  
  //Get 'Buy' order

  //Communicate with UPS - Tell which ware house to go to. Receive response with tracking num - GPB

  //Communicate with WH - GPB (Check inventory, if not there, order)

  //Add amount ordered, product description to our DB

  //Wait for 'ready' and 'arrived' from UPS - GPB

  //Tell WH to load packages, wait for reponse to tell you it's done

  //Tell UPS that packages are loaded

  C->disconnect();
  return 0;
}


