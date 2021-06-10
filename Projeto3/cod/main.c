/*  
João Gomes Maurício Nº98530

Jaime Rolo Costa Nº95595

*/



#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include "fs/operations.h"
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>
#include "tecnicofs-api-constants.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/uio.h>
#include <sys/stat.h>


#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100
#define INDIM 30
#define OUTDIM 512

int numberThreads = 0;
char * synchstrategy;
int sockfd;
socklen_t addrlen;

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int numberCommands = 0;
int headQueue = 0;

pthread_mutex_t mutex;        

int insertCommand(char* data) {
    if(numberCommands != MAX_COMMANDS) {
        strcpy(inputCommands[numberCommands++], data);
        return 1;
    }
    return 0;
}

char* removeCommand() {

    if(numberCommands > 0){
        numberCommands--;
        return inputCommands[headQueue++];
    }

    return NULL;
}

void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}

void processInput(char *name_file){
    char line[MAX_INPUT_SIZE];
    FILE *file_input;
    file_input = fopen(name_file,"r");

   if(file_input == NULL){
     perror(name_file);
     exit(1);
   }
      /* break loop with ^Z or ^D */
    while (fgets(line, sizeof(line)/sizeof(char), file_input)){
        char token, type;
        char name[MAX_INPUT_SIZE];
      /* release allocated memory */
        int numTokens = sscanf(line, "%c %s %c", &token, name, &type);

        /* perform minimal validation */
        if (numTokens < 1) {
            continue;
        }

        switch (token) {
            case 'c':
                if(numTokens != 3)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;

            case 'l':
                if(numTokens != 2)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;

            case 'd':
                if(numTokens != 2)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;

            case '#':
                break;

            default: { /* error */
                errorParse();
            }
        }
    }
    fclose(file_input);
}

void* applyCommands(){
    
    while (1){
    struct sockaddr_un client_addr;
    char in_buffer[INDIM]; 
    int c;
    int result;

    addrlen=sizeof(struct sockaddr_un);
    c = recvfrom(sockfd, in_buffer, sizeof(in_buffer)-1, 0,(struct sockaddr *)&client_addr, &addrlen);
    if (c < 0) {
        perror("ERROR: Server can\'t receive message");
        exit(EXIT_FAILURE);
    }
        char token; 
        char dir[MAX_INPUT_SIZE]; 
        char name[MAX_INPUT_SIZE];
        int numTokens = sscanf(in_buffer, "%c %s %s", &token, name, dir);
        if (numTokens < 2) {
            fprintf(stderr, "Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        }

        switch (token) {
            case 'c':
                switch (dir[0]) {
                    case 'f':
                        result = create(name, T_FILE);
                        break;
                    case 'd':
                        result = create(name, T_DIRECTORY);
                        break;
                    default:
                        fprintf(stderr, "Error: invalid node type\n");
                        exit(EXIT_FAILURE);
                }
                break;
            case 'l': 
                result = lookup(name);
                break;
            case 'd':
                result = delete(name);
                break;
            case 'm':
                pthread_mutex_lock(&mutex); 
                result = move(name, dir);
                pthread_mutex_unlock(&mutex); 
                break;   
            case 'p':
                pthread_mutex_lock(&mutex); 
                result = print(name);
                pthread_mutex_unlock(&mutex); 
                break;
            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }
        }

    printf("Recebeu mensagem de %s\n", client_addr.sun_path);
    
    if ((sendto(sockfd, &result, sizeof(int), 0, (struct sockaddr *)&client_addr, addrlen))<0){
        perror("ERROR: Server can\'t send message");
        exit(EXIT_FAILURE);
    }
    
    }
    return NULL;
}


int setSockAddrUn(char *path, struct sockaddr_un *addr) {

  if (addr == NULL)
    return 0;

  bzero((char *)addr, sizeof(struct sockaddr_un));
  addr->sun_family = AF_UNIX;
  strcpy(addr->sun_path, path);

  return SUN_LEN(addr);
}

int main(int argc, char** argv){
    struct sockaddr_un server_addr;
    int i;    
    numberThreads=atoi(argv[1]); 
    
    init_fs();

    if (argc < 3){
    exit(EXIT_FAILURE);
    }

    if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
    perror("server: can't open socket");
    exit(EXIT_FAILURE);
    }

    unlink(argv[2]);

    addrlen = setSockAddrUn(argv[2], &server_addr);

    if (bind(sockfd, (struct sockaddr *) &server_addr, addrlen) < 0) {
    perror("server: bind error");
    exit(EXIT_FAILURE);
    }


    if(numberThreads<=0){
      printf("Error: the number of threads must be a positive integer\n");
      return 0;
    }
    
    pthread_mutex_init(&mutex,NULL); 
    
    pthread_t tid[numberThreads];

    

    for( i=0; i< numberThreads; i++ ){
         if( pthread_create(&tid[i] , 0, applyCommands, NULL) != 0  ){
         printf("Error creating the thread\n");
         return -1;
      }
    }

    for( i=0; i< numberThreads; i++ ){
      if(pthread_join(tid[i], NULL)!=0){
      printf("Error joining thred.\n");
      return -1;
      }
    }

    pthread_mutex_destroy(&mutex); 
    destroy_fs();
    

    return 0;

    exit(EXIT_SUCCESS);

}
