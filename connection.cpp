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

using namespace pqxx;

#define AMAZON_PORT 23456
#define UPS_PORT 34567
//#define INT_MAX 9223372036854775807
#define WORLD_ID 1004

// global socket variable
int amazon_socket;
int ups_socket;
int64_t productID = -1;

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

void create_amazon_world_socket(){
  int option = 1;
  int amazon_socket;
  const char * host = "127.0.0.1";
  struct hostent * destination_server;
  struct sockaddr_in destination_server_addr;
  
  amazon_socket = socket(AF_INET, SOCK_STREAM, 0);
  if(amazon_socket < 0){
    perror("unable to create socket");
    exit(1);
  }

  /*
  struct sockaddr_in serveraddr;
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons(AMAZON_PORT);

  setsockopt(amazon_socket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
  */
  destination_server = gethostbyname(host);

  // memset stuff
  memset(&destination_server_addr,0,sizeof(destination_server_addr));
  destination_server_addr.sin_family = AF_INET;

  memcpy(&destination_server_addr.sin_addr.s_addr, destination_server->h_addr, destination_server->h_length);
  destination_server_addr.sin_port = htons(AMAZON_PORT);
  
  if(connect(amazon_socket,(struct sockaddr *)& destination_server_addr, sizeof(destination_server_addr)) < 0){
    perror("connection failed");
  }
  
  // format GPB message
  AConnect amazon_connect;
  amazon_connect.set_worldid(WORLD_ID);
  google::protobuf::io::FileOutputStream simout(amazon_socket);
  google::protobuf::io::FileInputStream simin(amazon_socket);
  if(sendMesgTo(amazon_connect, &simout)){
    std::cout << "connection to world initiated...\n";
  }
  AConnected aconnected;
  if(recvMesgFrom(aconnected, &simin)){
    std::cout << "connected!\n";
  }
}

int create_ID(){
  productID++;
  return productID;
}

void buy_request(){
  ACommands buyCommand;
  APurchaseMore *itemDes = buyCommand.add_buy();
  AProduct *product = itemDes->add_things(); 
  itemDes->set_whnum(0);
  product->set_id(10);
  product->set_description("supersuperduperduper");
  product->set_count(10);
  buyCommand.set_simspeed(100);

  google::protobuf::io::FileOutputStream simout(amazon_socket);
  google::protobuf::io::FileInputStream simin(amazon_socket);
  if(sendMesgTo(buyCommand, &simout)){
    std::cout << "new buy request sent to world!\n";
  }
  AResponses buyResponse;
  if(recvMesgFrom(buyResponse, &simin)){
    std::cout << "received confirmation of buy request from world!\n";
  }

  //std::cout << buyResponse << std::endl;
  // at this point the product has been bought and arrived at the warehouse

  /*
  // UPS stuff
  int ups_socket;
  const char * host = "127.0.0.1"; // assume we have a local copy of the UPS executable
  struct hostent * destination_server;
  struct sockaddr_in destination_server_addr;
  
  ups_socket = socket(AF_INET, SOCK_STREAM, 0);
  if(ups_socket < 0){
    perror("unable to create socket for ups");
    exit(1);
  }

  destination_server = gethostbyname(host);

  // memset stuff
  memset(&destination_server_addr,0,sizeof(destination_server_addr));
  destination_server_addr.sin_family = AF_INET;

  memcpy(&destination_server_addr.sin_addr.s_addr, destination_server->h_addr, destination_server->h_length);
  destination_server_addr.sin_port = htons(UPS_PORT);
  
  if(connect(amazon_socket,(struct sockaddr *)& destination_server_addr, sizeof(destination_server_addr)) < 0){
    perror("connection failed");
    exit(1);
  }

  AProduct newBuyRequest;
  newBuyRequest.set_id(5); // replace with actual product_id
  newBuyRequest.set_description("hehehehe"); //replace with actual description
  newBuyRequest.set_count(10); // replace with actual count

  google::protobuf::io::FileOutputStream simout2(ups_socket);
  google::protobuf::io::FileInputStream simin2(ups_socket);
  if(sendMesgTo(newBuyRequest, &simout2)){
    std::cout << "new buy request sent to UPS!\n";
  }

  AResponses UPSResponse;
  if(recvMesgFrom(UPSResponse, &simin2)){
    std::cout << "received confirmation from UPS!\n";
  }
  */
  
}

int main(){  
  create_amazon_world_socket();
  buy_request();
  return 0;
}
