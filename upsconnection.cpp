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
#include "ups.pb.h"
#include "ups.pb.cc"
#include "db.hpp"
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>
#include <pqxx/pqxx>
#include <postgresql/libpq-fe.h>
#include <sstream>
#include <sys/wait.h>

using namespace pqxx;

#define UPS 12345
#define AMAZON_PORT 23456
#define UPS_PORT 34567
#define WORLD_ID 1005
#define SIM_SPEED 100000000

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

void create_ups_world_socket(int * ups_world){
  int option = 1;
  const char * host = "127.0.0.1";
  struct hostent * destination_server;
  struct sockaddr_in destination_server_addr;

  *ups_world = socket(AF_INET, SOCK_STREAM, 0);
  if(*ups_world < 0){
    perror("unable to create socket");
    exit(1);
  }

  destination_server = gethostbyname(host);

  memset(&destination_server_addr,0,sizeof(destination_server_addr));
  destination_server_addr.sin_family = AF_INET;

  memcpy(&destination_server_addr.sin_addr.s_addr, destination_server->h_addr, destination_server->h_length);
  destination_server_addr.sin_port = htons(UPS);

  if(connect(*ups_world,(struct sockaddr *)& destination_server_addr, sizeof(destination_server_addr)) < 0){
    perror("connection failed");
  }
}

void connect_to_world(google::protobuf::io::FileOutputStream * simout, google::protobuf::io::FileInputStream * simin){
  // format GPB message
  UConnect ups_connect;
  ups_connect.set_reconnectid(WORLD_ID);
  if(sendMesgTo(ups_connect, simout)){
    std::cout << "connection to world initiated...\n";
  }
  UConnected aconnected;
  if(recvMesgFrom(aconnected, simin)){
    //nothing should be printed for error
    if (aconnected.has_error()) {
      std::cout << "[ra]"<< aconnected.error() << "\n";
    }
    std::cout << "connected!\n";
  }
}

void create_ups_amazon_socket(int *ups_amazon, int *amazon_port){
  const char * hostname = "127.0.0.1";
  int status;
  struct sockaddr_in serv_addr, cli_addr;
  socklen_t clilen;

  *ups_amazon = socket(AF_INET, SOCK_STREAM, 0);
  if(*ups_amazon < 0){
    perror("failed to open ups_amazon socket");
  }

  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(UPS_PORT);

  if (bind(*ups_amazon, (struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0){
    perror("error binding ups_amazon");
  }
  listen(*ups_amazon, 5);
  clilen = sizeof(cli_addr);
  *amazon_port = accept(*ups_amazon,(struct sockaddr *) &cli_addr,&clilen);
  std::cout << "successfully connected to amazon\n";
}

void recv_amazon(google::protobuf::io::FileOutputStream *simout2, google::protobuf::io::FileInputStream simin2){
  
}

void start_ups(google::protobuf::io::FileOutputStream *simout2, google::protobuf::io::FileInputStream simin2){
  pid_t pid = fork();
  int status;
  pid_t w;
  if(pid == 0){// child process
    recv_amazon(simout2, simin2);
  }
  else{// parent process
    w = waitpid(pid, &status, WUNTRACED | WCONTINUED);
  }
}

int main(){
  int ups_world;
  int ups_amazon;
  int amazon_port;
  //create_ups_world_socket(&ups_world);
  //google::protobuf::io::FileOutputStream simout(ups_world);
  //google::protobuf::io::FileInputStream simin(ups_world);
  //connect_to_world(&simout, &simin);
  create_ups_amazon_socket(&ups_amazon, &amazon_port);
  google::protobuf::io::FileOutputStream simout2(amazon_port);
  google::protobuf::io::FileInputStream simin2(amazon_port);
  start_ups(&simout2, &simin2);
}

