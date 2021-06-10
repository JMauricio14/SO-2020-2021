/*  
João Gomes Maurício Nº98530

Jaime Rolo Costa Nº95595

*/

#include "tecnicofs-client-api.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

int sockfd; 
socklen_t servlen; 
struct sockaddr_un serv_addr;

int setSockAddrUn(char *path, struct sockaddr_un *addr) {

  if (addr == NULL)
    return 0;

  bzero((char *)addr, sizeof(struct sockaddr_un));
  addr->sun_family = AF_UNIX;
  strcpy(addr->sun_path, path);

  return SUN_LEN(addr);
}


int tfsCreate(char *filename, char nodeType){
  
  int result;
  char command[MAX_INPUT_SIZE];
  char aux[3] = {' ', nodeType};
  strcpy(command, "c ");

  strcat(command, filename);
  strcat(command, aux);

  if (sendto(sockfd, command, strlen(command) + 1, 0, (struct sockaddr *)&serv_addr, servlen) < 0){
    perror("ERROR: client can\'t send message "); 
    return -1;
  }
  
  if ((recvfrom(sockfd,&result,sizeof(int),0,0,0))<0){
    perror("ERROR: client can\'t receive message "); 
    return -1;
  }
  
  return result;
}

int tfsDelete(char *path){
  int result;
  char command[MAX_INPUT_SIZE];
  strcpy(command, "d ");
  strcat(command, path);

 if ((sendto(sockfd, command, sizeof(char) * (strlen(command) + 1), 0, (struct sockaddr *)&serv_addr, servlen))<0){
    perror("ERROR: client can\'t send message "); 
    return -1;
  }

  if ((recvfrom(sockfd,&result,sizeof(int),0,0,0))<0){
    perror("ERROR: client can\'t receive message "); 
    return -1;
  }
  
  return result;
}

int tfsMove(char *from, char *to){
  int result;
  char command[MAX_INPUT_SIZE];
  strcpy(command, "m ");
  strcat(command, from);
  strcat(command, " ");
  strcat(command, to);
  
  if ((sendto(sockfd, command, sizeof(char) * (strlen(command) + 1), 0, (struct sockaddr *)&serv_addr, servlen))<0){
    perror("ERROR: client can\'t send message "); 
    return -1;
  }

  if ((recvfrom(sockfd,&result,sizeof(int),0,0,0))<0){
    perror("ERROR: client can\'t receive message "); 
    return -1;
  }
  
  return result;
}

int tfsLookup(char *path){
  int result;
  char command[MAX_INPUT_SIZE];
  strcpy(command, "l ");
  strcat(command, path);
  
  if ((sendto(sockfd, command, sizeof(char) * (strlen(command) + 1), 0, (struct sockaddr *)&serv_addr, servlen))<0){
    perror("ERROR: client can\'t send message "); 
    return -1;
  }

  if ((recvfrom(sockfd,&result,sizeof(int),0,0,0))<0){
    perror("ERROR: client can\'t receive message "); 
    return -1;
  }
  
  return result;
}

int tfsMount(char * sockPath){
  char buffer[1024]; 
  socklen_t clilen; 
  struct sockaddr_un client_addr; 

  if( ( sockfd = socket(AF_UNIX, SOCK_DGRAM, 0) ) < 0 ){
    perror("ERROR: client cant open socket "); 
    exit(EXIT_FAILURE); 
  }
  
  
  sprintf(buffer, "/tmp/client%d", getpid());

  unlink(buffer);
  
  clilen = setSockAddrUn(buffer, &client_addr); 

  if( (bind(sockfd, (struct sockaddr *) &client_addr, clilen))  <0){
     perror("ERROR -> on bind"); 
     exit(EXIT_FAILURE); 
  }
  
  servlen = setSockAddrUn(sockPath, &serv_addr); 

  return 0;
}

int tfsUnmount(){
  close(sockfd);

  char buffer[1024]; 
  
  sprintf(buffer, "/tmp/client%d", getpid());

  unlink(buffer);

  exit(EXIT_SUCCESS);
}

int tfsPrint(char *path){
    int result;
    char command[MAX_INPUT_SIZE];
    strcpy(command, "p ");
    strcat(command, path);

   if ((sendto(sockfd, command, sizeof(char) * (strlen(command) + 1), 0, (struct sockaddr *)&serv_addr, servlen))<0){
    perror("ERROR: client can\'t send message "); 
    return -1;
    }

    if ((recvfrom(sockfd,&result,sizeof(int),0,0,0))<0){
    perror("ERROR: client can\'t receive message "); 
    return -1;
    }
  
  return result;
}