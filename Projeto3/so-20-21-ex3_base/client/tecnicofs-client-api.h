#ifndef API_H
#define API_H

#include "tecnicofs-api-constants.h"

int tfsCreate(char *filename, char nodeType);
int tfsDelete(char *path);
int tfsLookup(char *path);
int tfsMove(char *from, char *to);
int tfsMount(char* serverName);
int tfsUnmount();
int tfsPrint(char *path); 

#endif /* CLIENT_H */
