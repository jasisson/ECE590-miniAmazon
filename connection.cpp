#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#define AMAZON_PORT 23456


// global socket variable
int amazon_socket;
int optval;

int create_amazon_world_socket(){
  int amazon_socket;
  struct sockaddr_in serv_addr;
  struct hostent *server;
  
  amazon_socket = socket(AF_INET, SOCK_STREAM, 0);
  if(amazon_socket == -1){
    perror("amazon socket failed to create");
  }

  if(setsockopt(amazon_socket,SOL_SOCKET,SO_REUSEADDR, &optval,sizeof(optval)) < 0){
    perror("setting socket options failed");
  }

  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);

  serv_addr.sin_port = htons(AMAZON_PORT);
  if (connect(amazon_socket,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0){
    perror("ERROR connecting");
  }


  return amazon_socket;
}

int main(){
  amazon_socket = create_amazon_world_socket();
  return 0;
}
