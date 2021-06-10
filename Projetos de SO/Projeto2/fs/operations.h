#ifndef FS_H
#define FS_H
#include "state.h"

void init_fs();
void destroy_fs();
int is_dir_empty(DirEntry *dirEntries);
int create(char *name, type nodeType, int inumber[INODE_TABLE_SIZE]);
int delete(char *name, int inumber[INODE_TABLE_SIZE]);
int lookup(char *name, int inumber[INODE_TABLE_SIZE], int permition);
void print_tecnicofs_tree(FILE *fp);

#endif /* FS_H */
