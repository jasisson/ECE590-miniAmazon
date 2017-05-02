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
#define WORLD_ID 1005
#define SIM_SPEED 100000000

// global socket variables
int ups_socket;
int64_t productID = -1;
int insertDB = 1;

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
  const char * host = "127.0.0.1";
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

int create_ID(){
  productID++;
  return productID;
}

void buy_request(int pid, int whnum, int q, string desc, google::protobuf::io::FileOutputStream * simout, google::protobuf::io::FileInputStream * simin){
  ACommands buyCommand;
  APurchaseMore *itemDes = buyCommand.add_buy();
  AProduct *product = itemDes->add_things();
  //std::cout << pid << std::endl;
  //std::cout << q << std::endl;
  //std::cout << whnum << std::endl;
  //std::cout << desc << std::endl;
  itemDes->set_whnum(whnum);
  product->set_id(pid);
  product->set_description(desc);
  product->set_count(q);
  buyCommand.set_simspeed(SIM_SPEED);
 
  if(sendMesgTo(buyCommand, simout)){
    std::cout << "new buy request sent to world for product (" << desc << ")\n"; 
  }
}

void receive_response(PGconn *dbconn, google::protobuf::io::FileOutputStream * simout, google::protobuf::io::FileInputStream * simin){
  AResponses response;
  if(recvMesgFrom(response, simin)){
    if(response.has_error()){
      std::cout << "[ra]"<< response.error() << "\n";
    }
    else{
      int total_arrived = response.arrived_size();
      if(total_arrived > 0){
	//std::cout << "total arrived: " << total_arrived << std::endl;
	for(int i = 0; i < total_arrived; i++){
	  int whnum = response.arrived(i).whnum();
	  //std::cout << "whnum: " << response.arrived(i).whnum()  << std::endl;
	  int product_size = response.arrived(i).things_size();
	  for(int j = 0; j < product_size; j++){
	    int id = response.arrived(i).things(j).id();
	    int count = response.arrived(i).things(j).count();
	    string description = response.arrived(i).things(j).description();
	    //std::cout << "id: " << id << std::endl;
	    //std::cout << "count: " << count << std::endl;
	    //std::cout << "description: " << description << std::endl;
	    ACommands packThese;
	    APack *package = packThese.add_topack();
	    AProduct *packageInfo = package->add_things();
	    packageInfo->set_id(id);
	    packageInfo->set_description(description);
	    packageInfo->set_count(count);
	    package->set_whnum(whnum);
	    packThese.set_simspeed(SIM_SPEED);
	    std::stringstream command;
	    //std::cout << "pid: " << id << " and count: " << count << std::endl;
	    PGresult *all;
	    string givemeall= "SELECT * FROM orders";
	    all = PQexec(dbconn, givemeall.c_str());
	    //std::cout << "total rows in DB: " << PQntuples(all) << std::endl;
	    command << "SELECT * FROM orders WHERE pid = " << id << " and q = " << count << """";
	    //std::cout << "command: " << command.str() << std::endl;
	    PGresult *query;
	    query = PQexec(dbconn, command.str().c_str());
	    int total_rows = PQntuples(query);
	    //std::cout << "total rows returned from query: " << total_rows << std::endl;
	    int order_no = atoi(PQgetvalue(query, 0, 10));
	    //std::cout << "order_no: " << order_no << std::endl;
	    package->set_shipid(order_no);
	    if(sendMesgTo(packThese, simout)){
	      std::cout << "new pack request sent to world for product (" << description << ") which has order number: " << order_no << std::endl; 
	    }
	  }
	}
      }
      int total_ready = response.ready_size();
      std::cout << "total ready: " << total_ready << std::endl;
      if(total_ready > 0){
	for(int k = 0; k < total_ready; k++){
	  int shipid = response.ready(k);
	  /*UPSResponses truck;
	  if(recvMesgFrom(truck, simin2)){
	    std::cout << "received message from UPS that truck " << truck.resp_truck.truckid() << " has arrived\n";
	  }
	  int truckid = truck.resp_truck.truckid();
	  int whnum = truck.resp_truck.whnum();
	  int shipid = truck.resp_truck.shipid();
	  */
	  ACommands loadThese;
	  APutOnTruck *package = loadThese.add_load();
	  package->set_truckid(0); //get actual truck_id
	  package->set_whnum(shipid);
	  package->set_shipid(shipid);
	  if(sendMesgTo(loadThese, simout)){
	    std::cout << "sent message to world to load item with order number: " << shipid << std::endl;
	  }
	}
      }
      int total_loaded = response.loaded_size();
      std::cout << "total loaded: " << total_loaded << std::endl;
      if(total_loaded > 0){
	for(int l = 0; l < total_loaded; l++){
	  int number = response[l].loaded();
	  std::stringstream status;
	  status << "UPDATE orders SET status = 2 WHERE order_no = " << number << """";
	  PQexec(dbconn, status.str().c_str());
	  // send UPS msg that we are ready to deliver
	}
      }
    }
  }
}

void initialize_database(PGconn *dbconn){
  if(insertDB){
    string del = "DELETE FROM orders";
    PQexec(dbconn, del.c_str());
    string command = "INSERT INTO orders VALUES(1,1,10,'iphone',0,-1,2,3,105,'tech',1)";
    PQexec(dbconn, command.c_str());
    string command2 = "INSERT INTO orders VALUES(2,2,5,'laptop',0,-1,100,50,100,'tech',2)";
    PQexec(dbconn, command2.c_str());
  }
}

void start_amazon(PGconn *dbconn, google::protobuf::io::FileOutputStream * simout, google::protobuf::io::FileInputStream * simin){
  pid_t pid = fork();
  int status;
  pid_t w;
  if(pid == 0){ // child process
    //while(1){
      // get all the orders that are fresh (i.e. all orders the backend hasn't seen)
      string command = "SELECT * FROM orders WHERE status = 0";
      PGresult *query;
      query = PQexec(dbconn, command.c_str());
      int total_rows = PQntuples(query);
      //std::cout << total_rows << std::endl;
      for(int i = 0; i < total_rows; i++){
	int order_no = atoi(PQgetvalue(query, i, 10));
	std::stringstream command;
	command << "UPDATE orders SET status = 1 WHERE order_no = " << order_no << """";
	PQexec(dbconn, command.str().c_str());
	int pid = atoi(PQgetvalue(query, i, 0));
	int whnum = atoi(PQgetvalue(query, i, 1));
	int q = atoi(PQgetvalue(query, i, 2));
	string desc = PQgetvalue(query, i, 3);
	int addx = atoi(PQgetvalue(query, i, 6));
	int addy = atoi(PQgetvalue(query, i, 7));
	buy_request(pid, whnum, q, desc, simout, simin);
      }
      //}
  }
  else if(pid > 0){ // parent
    while(1){
      receive_response(dbconn, simout, simin);
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
  //create_amazon_ups_socket(&ups_socket);
  google::protobuf::io::FileOutputStream simout(amazon_socket);
  google::protobuf::io::FileInputStream simin(amazon_socket);
  //google::protobuf::io::FileOutputStream simout2(ups_socket);
  //google::protobuf::io::FileInputStream simin2(ups_socket);
  connect_to_world(&simout, &simin);
  // query the database to see if there are any new buy requests  
  PGconn *dbconn;
  dbconn = PQconnectdb("dbname=localdb user=postgres password=passw0rd");
  if(PQstatus(dbconn) == CONNECTION_BAD){
    perror("unable to connect to db");
  }
  initialize_database(dbconn);
  start_amazon(dbconn, &simout, &simin);
  //obtain_buy_request(dbconn);
  //buy_request(&simout, &simin, &simout2, &simin2);
  //load_request(&simout, &simin, &simout2, &simin2);
  //pack_request(&simout, &simin, &simout2, &simin2);
  //PQfinish(dbconn);
  return 0;
}
