#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include "amazon.pb.h"
#include "amazon.pb.cc"
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>

#define AMAZON_PORT 23456

// global socket variable
int amazon_socket;
int optval;

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

  struct sockaddr_in serveraddr;
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons(AMAZON_PORT);

  setsockopt(amazon_socket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

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
  amazon_connect.set_worldid(1003);
  //amazon_connect.set_inithw(1);
  google::protobuf::io::FileOutputStream simout(amazon_socket);
  google::protobuf::io::FileInputStream simin(amazon_socket);
  sendMesgTo(amazon_connect, &simout);
  AConnected aconnected;
  recvMesgFrom(aconnected, &simin);
  // nothing should print if connecting succeeded
  std::cout << aconnected << std::endl;
}

int main(){
  create_amazon_world_socket();
  return 0;
}
