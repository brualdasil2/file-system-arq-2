#include <stdio.h>
#include <stdlib.h>

#define VAZIO 0xFF
#define END_OF_FILE 0xFE
#define CORROMPIDO 0xFD

#define TAM_CLUSTER 32768
#define TAM_INDICE 256
#define I_INDICE 8
#define I_ROOT 264

typedef struct {
    char workingDir[221];
    char workingDirIndex;
} DIR_STATE;

DIR_STATE dirState;

typedef struct {
    char nome[20];
    char tipo[4];
} CLUSTER;

typedef struct {
    unsigned short tam_indice;
    unsigned short tam_cluster;
    unsigned short ini_indice;
    unsigned short ini_root;
} META_PROGRAMA;

typedef struct {
    char* tabela;
} INDICE;

typedef struct {
    META_PROGRAMA meta;
    INDICE indice;
    CLUSTER clusters[TAM_INDICE];
    FILE* arquivo;
} FS;

void initCluster();

FS initFS();

unsigned char getDirIndex(char* path, FS fileSystem);

void cd(char* path, FS fileSystem);

void dir(FS fileSystem);

void rm(char* path, FS fileSystem);

void mkdir(char* name, FS fileSystem);

void mkfile(char* name, FS fileSystem);

void edit(char* path, char* text, FS fileSystem);

void move(char* srcPath, char* destPath, FS fileSystem);

void rename(char* path, char* name, FS fileSystem);




