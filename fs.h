#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VAZIO 0xFF
#define END_OF_FILE 0xFE
#define CORROMPIDO 0xFD

#define TAM_CLUSTER 32768
#define TAM_INDICE 256
#define I_INDICE 8
#define I_ROOT 264
#define MAX_CHAR TAM_CLUSTER - sizeof(CLUSTER) - 1
#define MAX_PATH 256

#define NOME_ARQUIVO "data.bin"


typedef struct {
    char workingDir[MAX_PATH];
    unsigned char workingDirIndex;
} DIR_STATE;

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
    META_PROGRAMA meta;
    char* indice;
    CLUSTER* clusters;
    FILE* arquivo;
    DIR_STATE dirState;
} FS;

void initCluster();

FS initFS();

void cd(char* path, FS* fileSystem);

void dir(FS fileSystem);

void rm(char* path, FS *fileSystem);

void mkdir(char* name, FS* fileSystem);

void mkfile(char* name, FS* fileSystem);

void edit(char* path, char* text, FS* fileSystem);

void move(char* srcPath, char* destPath, FS *fileSystem);

void renameFile(char* path, char* name, FS* fileSystem);

