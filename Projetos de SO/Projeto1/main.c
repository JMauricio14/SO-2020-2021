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


#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100

int numberThreads = 0;
char * synchstrategy;

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int numberCommands = 0;
int headQueue = 0;

pthread_mutex_t mutex_lock1, mutex_lock2;       /*Inicializar as Variaveis mutex globais*/
pthread_rwlock_t rwlock_lock1;                  /*Inicializar a variavel rwlock global*/


int insertCommand(char* data) {
    if(numberCommands != MAX_COMMANDS) {
        strcpy(inputCommands[numberCommands++], data);
        return 1;
    }
    return 0;
}

char* removeCommand() {

    pthread_mutex_lock(&mutex_lock2);       /*Dar lock*/

    if(numberCommands > 0){
        numberCommands--;
        pthread_mutex_unlock(&mutex_lock2);
        return inputCommands[headQueue++];
    }

    pthread_mutex_unlock(&mutex_lock2);     /*Dar unlock*/
    return NULL;
}

/*Funçao auxilar para a Estrategia nosync*/
void NOSYNC(){
  return;
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

    while (numberCommands > 0){

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

        int searchResult;
        switch (token){
            case 'c':
                    /*Dar lock*/
                if (!strcmp(synchstrategy, "mutex")){
                    pthread_mutex_lock(&mutex_lock1);
                }
                else if (!strcmp(synchstrategy, "rwlock")) {
                    pthread_rwlock_wrlock(&rwlock_lock1);
                }else if(!strcmp(synchstrategy, "nosync") && numberThreads == 1){
                  NOSYNC();
                }

                switch (type) {                                                /*inicio da secção critica*/
                    case 'f':
                        printf("Create file: %s\n", name);
                        create(name, T_FILE);
                        break;
                    case 'd':
                        printf("Create directory: %s\n", name);
                        create(name, T_DIRECTORY);
                        break;
                    default:
                        fprintf(stderr, "Error: invalid node type\n");
                        exit(EXIT_FAILURE);
                }

               /*Dar unlock*/
                if (!strcmp(synchstrategy, "mutex")){
                    pthread_mutex_unlock(&mutex_lock1);
                }
                else if (!strcmp(synchstrategy, "rwlock")) {
                    pthread_rwlock_unlock(&rwlock_lock1);
                }else if(!strcmp(synchstrategy, "nosync") && numberThreads == 1){
                  NOSYNC();
                }

                break;
            case 'l':

                /*Dar lock*/
                if (!strcmp(synchstrategy, "mutex")){
                    pthread_mutex_lock(&mutex_lock1);
                }
                else if (!strcmp(synchstrategy, "rwlock")) {
                    pthread_rwlock_rdlock(&rwlock_lock1);
                }else if(!strcmp(synchstrategy, "nosync")&& numberThreads == 1){
                  NOSYNC();
                }

                searchResult = lookup(name);                         /*inicio da secção critica*/

                if (searchResult >= 0)
                    printf("Search: %s found\n", name);
                else
                    printf("Search: %s not found\n", name);

                /*Dar unlock*/
                if (!strcmp(synchstrategy, "mutex")){
                    pthread_mutex_unlock(&mutex_lock1);
                }
                else if (!strcmp(synchstrategy, "rwlock")) {
                    pthread_rwlock_unlock(&rwlock_lock1);
                }else if(!strcmp(synchstrategy, "nosync")&& numberThreads == 1){
                  NOSYNC();
                }

                break;
            case 'd':

                printf("Delete: %s\n", name);

                /*Dar lock*/
                if (!strcmp(synchstrategy, "mutex")){
                    pthread_mutex_lock(&mutex_lock1);
                }
                else if (!strcmp(synchstrategy, "rwlock")) {
                    pthread_rwlock_wrlock(&rwlock_lock1);
                }else if(!strcmp(synchstrategy, "nosync")&& numberThreads == 1){
                  NOSYNC();
                }

                delete(name);                                                  /*inicio da secção critica*/

                /*Dar unlock*/
                if (!strcmp(synchstrategy, "mutex")){
                    pthread_mutex_unlock(&mutex_lock1);
                }
                else if (!strcmp(synchstrategy, "rwlock")) {
                    pthread_rwlock_unlock(&rwlock_lock1);
                }else if(!strcmp(synchstrategy, "nosync")&& numberThreads == 1){
                  NOSYNC();
                }

                break;
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

    init_fs();

    if(argc != 5){
      printf("Wrong way to compile the program!\n");
      return 0;
    }


    FILE *file_output;
    file_output = fopen(argv[2],"w");

    if(file_output == NULL){
      perror(argv[2]);
      exit(1);
    }

    /* process input and print tree */
    processInput(argv[1]);


    numberThreads=atoi(argv[3]);

    if(numberThreads<=0){
      printf("Error: the number of threads must be a positive integer\n");
      return 0;
    }

    synchstrategy = (char *) malloc((sizeof(char) + 1) * strlen(argv[4]));
    strcpy(synchstrategy, argv[4]);

    if( strcmp(synchstrategy, "rwlock")!=0 && strcmp(synchstrategy, "mutex") != 0 && (strcmp(synchstrategy, "nosync")!=0)){
        printf("Wrong way to compile the program!\n");
        return 0;
    }

    if(strcmp(synchstrategy, "nosync")==0 && numberThreads!=1){
      printf("Error: The nosync only accepts 1 thread!\n");
      return 0;
    }

    //inicializar tudo
    if (!strcmp(synchstrategy, "mutex")){
        pthread_mutex_init(&mutex_lock1, NULL);                           /*Mutex global*/
    }
    else if (!strcmp(synchstrategy, "rwlock")) {
        pthread_rwlock_init(&rwlock_lock1, NULL);                         /*rwlock global*/
    }else if(!strcmp(synchstrategy, "nosync")&& numberThreads == 1){
      NOSYNC();
    }

    pthread_mutex_init(&mutex_lock2, NULL);


    pthread_t tid[numberThreads];

    gettimeofday(&start, NULL);

    /*Inicio da criaçao de tarefas*/
    for( i=0; i< numberThreads; i++ ){
         if( pthread_create(&tid[i] , 0, applyCommands, NULL) != 0  ) {
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

    print_tecnicofs_tree(file_output);
    fclose(file_output);


    /* release allocated memory */
    pthread_mutex_destroy(&mutex_lock2);
    destroy_fs();
    free(synchstrategy);

    gettimeofday(&end, NULL);
    double time=(end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec);
    /*time in microsecs*/

    printf("TecnicoFS completed in %lf seconds\n", time/100000.0  );

    return 0;

    exit(EXIT_SUCCESS);

}
