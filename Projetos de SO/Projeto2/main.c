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


#define MAX_COMMANDS 10
#define MAX_INPUT_SIZE 100

int numberThreads = 0;


char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int numberCommands = 0;
int headQueue = 0;
int cheio=0; 
int done=0; 

pthread_cond_t empty; 
pthread_cond_t fill; 
pthread_mutex_t m; 


void put(char* command){
    strcpy(inputCommands[cheio], command); 
    cheio= (cheio + 1) % MAX_COMMANDS; 
    numberCommands++; 
}

char* get(){
    char*tmp = malloc(sizeof(char) * 1024); 
    strcpy(tmp, inputCommands[headQueue]); 
    headQueue= (headQueue + 1) % MAX_COMMANDS;
    numberCommands--; 
    return tmp; 
}

int insertCommand(char* data) {
    pthread_mutex_lock(&m); 
    while(numberCommands == MAX_COMMANDS){
        pthread_cond_wait(&empty, &m); 
    }
    put(data); 
    pthread_cond_signal(&fill); 
    pthread_mutex_unlock(&m);
    return 1;
}

char* removeCommand() {
    pthread_mutex_lock(&m);
    while(numberCommands==0){
        pthread_cond_wait(&fill, &m);
    }
    char* tmp = get();  
    pthread_cond_signal(&empty); 
    pthread_mutex_unlock(&m);
    return tmp;
}

void unlock(int inumber[INODE_TABLE_SIZE]){
  for(int i=0;i<INODE_TABLE_SIZE;i++){
       if(inumber[i] > 0 ){
        pthread_rwlock_unlock(&inode_table[i].trincoFino); 
       }
    }
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
    done=1;
    pthread_cond_broadcast(&fill); 

    fclose(file_input);
}

void* applyCommands(){

    while (1) {

        if (done == 1 && numberCommands == 0) break;

        const char* command = removeCommand();

        if (command == NULL){
            continue;
        }

        char token, type;
        char name[MAX_INPUT_SIZE];
        int numTokens = sscanf(command, "%c %s %c", &token, name, &type);
        if (numTokens < 2) {
            fprintf(stderr, "Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        }

        int numberINODE[INODE_TABLE_SIZE] = {-1};

        int searchResult;
        switch (token){
            case 'c':
                
                switch (type) { 
                                                       /*inicio da secção critica*/
                    case 'f':
                        
                        printf("Create file: %s\n", name);
                        create(name, T_FILE,numberINODE);
                        unlock(numberINODE);
                        break; 
                    case 'd':
                        
                        printf("Create directory: %s\n", name);
                        create(name, T_DIRECTORY,numberINODE);
                        unlock(numberINODE);
                        break; 
                    default:
                        fprintf(stderr, "Error: invalid node type\n");
                        exit(EXIT_FAILURE);
                }
                unlock(numberINODE);
                break;
            case 'l':

                searchResult = lookup(name, numberINODE,0); /*inicio da secção critica*/    
                
                if(searchResult==FAIL){
                    printf("ERROR: in search lookup\n"); 
                    exit(-1);   
                }

                if (searchResult >= 0)
                    printf("Search: %s found\n", name);
                else
                    printf("Search: %s not found\n", name);
                unlock(numberINODE);

                break; 
            case 'd':
                printf("Delete: %s\n", name);
                delete(name,numberINODE);
                unlock(numberINODE);
                break;                                                /*inicio da secção critica*/
            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }
        }
        
        
    }
    return NULL;
}


int main(int argc, char* argv[]){
    /* init filesystem */
    int i;
    struct timeval start, end;  /*Estrategia usada para obter o tempo de execução do programa*/
    if(argc != 4){
      printf("Wrong way to compile the program!\n");
      return 0;
    }

    init_fs();

    FILE *file_output;
    file_output = fopen(argv[2],"w");

   if(file_output == NULL){
      perror(argv[2]);
      exit(1);
    }

    /* process input and print tree */
    numberThreads=atoi(argv[3]);

    pthread_cond_init(&fill, NULL);
    pthread_cond_init(&empty, NULL);
    pthread_mutex_init(&m, NULL);


    if(numberThreads<=0){
      printf("Error: the number of threads must be a positive integer\n");
      return 0;
    }

    pthread_t tid[numberThreads];

    gettimeofday(&start, NULL);

    
    /*Inicio da criaçao de tarefas*/
    for( i=0; i< numberThreads; i++ ){
         if( pthread_create(&tid[i] , 0, applyCommands, NULL) != 0  ) {
         printf("Error creating the thread\n");
         return -1;
      }
    }

    processInput(argv[1]);

    for( i=0; i< numberThreads; i++ ){
      if(pthread_join(tid[i], NULL)!=0){
      printf("Error joining thred.\n");
      return -1;
    }
  }

    print_tecnicofs_tree(file_output);
    fclose(file_output);


    pthread_mutex_destroy(&m); 
    destroy_fs();


    gettimeofday(&end, NULL);
    double time=(end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec);
    /*time in microsecs*/

    printf("TecnicoFS completed in %lf seconds\n", time/1000000.0  );

    return 0;

    exit(EXIT_SUCCESS);

}
