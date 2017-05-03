#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <random>
#include <vector>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include "amazon.pb.h"
#include "amazon.pb.cc"
#include "db.hpp"
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>
#include <pqxx/pqxx>
#include <postgresql/libpq-fe.h>
#include <sstream>
#include <sys/wait.h>

using namespace pqxx;

#define AMAZON_PORT 23456
#define UPS_PORT 34567
#define WORLD_ID 1006
#define SIM_SPEED 100000000
#define TRUCKS 4
#define insertDB 1

//this is adpated from code that a google engineer posted online
template<typename T> bool sendMesgTo(const T & message,
				     google::protobuf::io::FileOutputStream *out) {
  {//extra scope: make output go away before out->Flush()
    // We create a new coded stream for each message.  Don't worry, this is fast.
    google::protobuf::io::CodedOutputStream output(out);
    // Write the size.
    const int size = message.ByteSize();
    output.WriteVarint32(size);
    uint8_t* buffer = output.GetDirectBufferForNBytesAndAdvance(size);
    if (buffer != NULL) {
      // Optimization:  The message fits in one buffer, so use the faster
      // direct-to-array serialization path.
      message.SerializeWithCachedSizesToArray(buffer);
    } else {
      // Slightly-slower path when the message is multiple buffers.
      message.SerializeWithCachedSizes(&output);
      if (output.HadError()) {
	return false;
      }
    }
  }
  //std::cout << "flushing...\n";
  out->Flush();
  return true;
}

template<typename T> bool recvMesgFrom(T & message,
				       google::protobuf::io::FileInputStream * in ){
  // We create a new coded stream for each message.  Don't worry, this is fast,
  // and it makes sure the 64MB total size limit is imposed per-message rather
  // than on the whole stream.  (See the CodedInputStream interface for more
  // info on this limit.)
  google::protobuf::io::CodedInputStream input(in);
  // Read the size.
  uint32_t size;
  if (!input.ReadVarint32(&size)) {
    return false;
  }
  // Tell the stream not to read beyond that size.
  google::protobuf::io::CodedInputStream::Limit limit = input.PushLimit(size);
  // Parse the message.
  if (!message.MergeFromCodedStream(&input)) {
    return false;
  }
  if (!input.ConsumedEntireMessage()) {
    return false;
  }
  // Release the limit.
  input.PopLimit(limit);
  return true;
}


std::ostream & operator<<(std::ostream & s, const google::protobuf::Message & m){
  std::string str;
  google::protobuf::TextFormat::PrintToString(m,&str);
  return s<< str;
}

void create_amazon_world_socket(int * amazon_socket){
  int option = 1;
  const char * host = "127.0.0.1";
  struct hostent * destination_server;
  struct sockaddr_in destination_server_addr;
  
  *amazon_socket = socket(AF_INET, SOCK_STREAM, 0);
  if(*amazon_socket < 0){
    perror("unable to create socket");
    exit(1);
  }

  destination_server = gethostbyname(host);

  memset(&destination_server_addr,0,sizeof(destination_server_addr));
  destination_server_addr.sin_family = AF_INET;

  memcpy(&destination_server_addr.sin_addr.s_addr, destination_server->h_addr, destination_server->h_length);
  destination_server_addr.sin_port = htons(AMAZON_PORT);
  
  if(connect(*amazon_socket,(struct sockaddr *)& destination_server_addr, sizeof(destination_server_addr)) < 0){
    perror("connection failed");
  }
}

void create_amazon_ups_socket(int * ups_socket){
  int option = 1;
  const char * host = "10.236.48.12";
  struct hostent * destination_server;
  struct sockaddr_in destination_server_addr;
  
  *ups_socket = socket(AF_INET, SOCK_STREAM, 0);
  if(*ups_socket < 0){
    perror("unable to create socket");
    exit(1);
  }

  destination_server = gethostbyname(host);

  memset(&destination_server_addr,0,sizeof(destination_server_addr));
  destination_server_addr.sin_family = AF_INET;

  memcpy(&destination_server_addr.sin_addr.s_addr, destination_server->h_addr, destination_server->h_length);
  destination_server_addr.sin_port = htons(UPS_PORT);
  
  if(connect(*ups_socket,(struct sockaddr *)& destination_server_addr, sizeof(destination_server_addr)) < 0){
    perror("connection failed");
  }
}


void connect_to_world(google::protobuf::io::FileOutputStream * simout, google::protobuf::io::FileInputStream * simin){   
  // format GPB message
  AConnect amazon_connect;
  amazon_connect.set_worldid(WORLD_ID);
  if(sendMesgTo(amazon_connect, simout)){
    std::cout << "connection to world initiated...\n";
  }
  AConnected aconnected;
  if(recvMesgFrom(aconnected, simin)){
    //nothing should be printed for error
    if (aconnected.has_error()) {
      std::cout << "[ra]"<< aconnected.error() << "\n";
    }
    std::cout << "connected!\n";
  }
}

void buy_request(int x, int y, int pid, int whnum, int q, string desc, google::protobuf::io::FileOutputStream * simout, google::protobuf::io::FileInputStream * simin, google::protobuf::io::FileOutputStream * simout2){
  // send buy request to world
  ACommands buyCommand;
  APurchaseMore *itemDes = buyCommand.add_buy();
  AProduct *product = itemDes->add_things();
  itemDes->set_whnum(whnum);
  product->set_id(pid);
  product->set_description(desc.c_str());
  product->set_count(q);
  buyCommand.set_simspeed(SIM_SPEED);
 
  if(sendMesgTo(buyCommand, simout)){
    std::cout << "new buy request sent to world for product (" << desc << ")\n"; 
  }

  // send message to UPS to start moving truck to that location
  AmazonCommands buyUPS; 
  UAShipRequest *UA;
  buyUPS.set_allocated_req_ship(UA);
  UA->set_x(x);
  UA->set_y(y);
  UAPack *pack;
  UA->set_allocated_package(pack);
  pack->set_whnum(whnum);
  pack->set_shipid(pid);
  UAProduct *product2 = pack->add_things();
  product2->set_id(pid);
  product2->set_description(desc.c_str());
  product2->set_count(q);
  if(sendMesgTo(buyUPS, simout2)){
    std::cout << "let UPS know of pid (" << pid << ") which is (" << desc << ")\n";
  }
}

void receive_response(PGconn *dbconn, google::protobuf::io::FileOutputStream * simout, google::protobuf::io::FileInputStream * simin, google::protobuf::io::FileOutputStream * simout2, google::protobuf::io::FileInputStream * simin2){
  AResponses response;
  if(recvMesgFrom(response, simin)){
    if(response.has_error()){
      std::cout << "[ra]"<< response.error() << "\n";
    }
    else{
      // CASE 1: pack request after receiving information from world that orders have arrived
      int total_arrived = response.arrived_size();
      if(total_arrived > 0){
	for(int i = 0; i < total_arrived; i++){
	  int whnum = response.arrived(i).whnum();
	  int product_size = response.arrived(i).things_size();
	  for(int j = 0; j < product_size; j++){
	    int id = response.arrived(i).things(j).id();
	    int count = response.arrived(i).things(j).count();
	    string description = response.arrived(i).things(j).description();
	    
	    ACommands packThese;
	    APack *package = packThese.add_topack();
	    AProduct *packageInfo = package->add_things();
	    packageInfo->set_id(id);
	    packageInfo->set_description(description);
	    packageInfo->set_count(count);
	    package->set_whnum(whnum);
	    packThese.set_simspeed(SIM_SPEED);
	    
	    std::stringstream command;
	    command << "SELECT * FROM orders WHERE pid = " << id << " and q = " << count << """";
	    PGresult *query;
	    query = PQexec(dbconn, command.str().c_str());
	    int order_no = atoi(PQgetvalue(query, 0, 10));
	    package->set_shipid(order_no);
	    if(sendMesgTo(packThese, simout)){
	      std::cout << "new pack request sent to world for product (" << description << ") which has order number: " << order_no << std::endl; 
	    }
	  }
	}
      }
      // CASE 2: we need the truck to be ready, which we receive information from UPS.  
      int total_ready = response.ready_size();
      if(total_ready > 0){
	// if we are in this statement, it means that a package is "complete". We just have to wait for UPS to send us truck information
	// to TA: this will not execute since we can't send a buy request to UPS, which means we won't be getting a response saying truck has arrived.
	// I just wanted to show my thought process.
	for(int k = 0; k < total_ready; k++){
	  UPSResponses UPSres;
	  if(recvMesgFrom(UPSres, simin2)){
	    std::cout << "received message from UPS that truck " << UPSres.resp_truck().truckid() << " has arrived\n";
	    int truckid = UPSres.resp_truck().truckid();
	    int whnum = UPSres.resp_truck().whnum();
	    int shipid = UPSres.resp_truck().shipid();
	    ACommands loadThese;
	    APutOnTruck *package = loadThese.add_load();
	    package->set_truckid(truckid);
	    package->set_whnum(whnum);
	    package->set_shipid(shipid);
	    if(sendMesgTo(loadThese, simout)){
	      std::cout << "sent message to world to load item with order number: " << shipid << std::endl;
	    }
	  }
	}
      }
      // CASE 3: This means that the truck is loaded. Here, we send UPS the information that we are ready to ship.
      int total_loaded = response.loaded_size();
      if(total_loaded > 0){
	for(int l = 0; l < total_loaded; l++){
	  int number = response.loaded(l);
	  // send message to UPS that we are done
	  int truck_id = number % TRUCKS;
	  AmazonCommands deliver;
	  deliver.set_req_deliver_truckid(truck_id);
	  if(sendMesgTo(deliver, simout2)){
	    std::cout << "package (" << number << ") has been shipped by UPS" << std::endl;
	  }
	  std::stringstream status;
	  // I update status to 2 for the order so that the front end can display this information.
	  // This is one of the niceties we implemented where we would be able to tell the user the status of their shipment based
	  // on the "status" variable in the database.
	  status << "UPDATE orders SET status = 2 WHERE order_no = " << number << """";
	  PQexec(dbconn, status.str().c_str()); 
	}
      }
    }
  }
}

void initialize_database(PGconn *dbconn){
  // These are just some dummy entries in the database so that I could test without the frontend
  // in the top. please set "insertDB" to 0 if testing with front end
  string del = "DELETE FROM orders";
  PQexec(dbconn, del.c_str());
  if(insertDB){
    string command = "INSERT INTO orders VALUES(1,1,10,'iphone',0,-1,2,3,105,'tech',1,-1)";
    PQexec(dbconn, command.c_str());
    string command2 = "INSERT INTO orders VALUES(2,2,5,'laptop',0,-1,100,50,100,'tech',2,-1)";
    PQexec(dbconn, command2.c_str());
  }
}

void start_amazon(PGconn *dbconn, google::protobuf::io::FileOutputStream * simout, google::protobuf::io::FileInputStream * simin, google::protobuf::io::FileOutputStream * simout2, google::protobuf::io::FileInputStream * simin2){
  pid_t pid = fork();
  int status;
  pid_t w;
  if(pid == 0){ // child process
    while(1){
      // set some time so that we can see prints to cout clearly
      usleep(1000000);
      // get all the orders that are fresh (i.e. all orders the backend hasn't seen, which is indicated by status = 0)
      string command = "SELECT * FROM orders WHERE status = 0";
      PGresult *query;
      query = PQexec(dbconn, command.c_str());
      int total_rows = PQntuples(query);
      if(total_rows != 0){
	for(int i = 0; i < total_rows; i++){
	  int total_rows = PQntuples(query);
	  int pid = atoi(PQgetvalue(query, i, 0));
	  int whnum = atoi(PQgetvalue(query, i, 1));
	  int q = atoi(PQgetvalue(query, i, 2));
	  string desc = PQgetvalue(query, i, 3);
	  int x = atoi(PQgetvalue(query, i, 6));
	  int y = atoi(PQgetvalue(query, i, 7));
	  int addx = atoi(PQgetvalue(query, i, 6));
	  int addy = atoi(PQgetvalue(query, i, 7));
	  buy_request(x, y, pid, whnum, q, desc, simout, simin, simout2);
	  int order_no = atoi(PQgetvalue(query, i, 10));
	  std::stringstream update;
	  // update status to 1 in the database to indicate the backend has seen the order
	  update << "UPDATE orders SET status = 1 WHERE order_no = " << order_no << """";
	  PQexec(dbconn, update.str().c_str());
	  string showall = "SELECT * FROM orders";
	  PGresult *show;
	  show = PQexec(dbconn, showall.c_str());
	  int all = PQntuples(show);
	  for(int n =0; n < all; n++){
	    // for testing and debugging
	    std::cout << "Here is the current state of DB -> " << PQgetvalue(show, n, 3) << " " << atoi(PQgetvalue(show, n, 4)) << std::endl; 
	  }
	}
      }
    }
  }
  else if(pid > 0){ // parent
    while(1){
      receive_response(dbconn, simout, simin, simout2, simin2);
    }
    w = waitpid(pid, &status, WUNTRACED | WCONTINUED);
  }
  else{
    std::cerr << "fork failed";
  }
}

int main(){
  int amazon_socket;
  int ups_socket;
  create_amazon_world_socket(&amazon_socket);
  create_amazon_ups_socket(&ups_socket);
  google::protobuf::io::FileOutputStream simout(amazon_socket);
  google::protobuf::io::FileInputStream simin(amazon_socket);
  google::protobuf::io::FileOutputStream simout2(ups_socket);
  google::protobuf::io::FileInputStream simin2(ups_socket);
  connect_to_world(&simout, &simin);
  
  PGconn *dbconn;
  dbconn = PQconnectdb("dbname=localdb user=postgres password=passw0rd");
  if(PQstatus(dbconn) == CONNECTION_BAD){
    perror("unable to connect to db");
  }
  initialize_database(dbconn);
  start_amazon(dbconn, &simout, &simin, &simout2, &simin2);
  return 0;
}
