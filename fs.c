#include "fs.h"

//Escreve 32kb de VAZIO no arquivo, o equivalente a um cluster inteiro não inizializado (sem nome), com um END_OF_FILE no início da área de dados
void initCluster(FILE* arquivo) {
    int i;
    char c = VAZIO;
    for (i = 0; i < sizeof(CLUSTER); i++) { //metadados com VAZIO
        fwrite(&c, sizeof(char), 1, arquivo);
    }
    c = END_OF_FILE;
    fwrite(&c, sizeof(char), 1, arquivo); //END_OF_FILE pra marcar o início da área de dados
    c = VAZIO;
    for (i = 0; i < TAM_CLUSTER - sizeof(CLUSTER) - 1; i++) { //área de dados com VAZIO
        fwrite(&c, sizeof(char), 1, arquivo);
    }
}

//Abre o arquivo se ele já existe, e retorna NULL caso não exista
FILE* fileExists(char* filename) {
    FILE* file;
    if (file = fopen(filename, "rb+")) {
        return file;
        fclose(file);
    }else{
        return NULL;
    }
}

//Inicializa o arquivo e as estruturas do FS
FS initFS() {
    FILE* arquivo;
    FS fileSystem;
    int i;
    if (arquivo = fileExists("data.bin")) {
        //se o arquivo já existe, só lê os dados e salva no FS
        fread(&(fileSystem.meta), sizeof(fileSystem.meta), 1, arquivo); //lê metadados
        fileSystem.indice = (char*)(malloc(sizeof(char)*TAM_INDICE)); //aloca espaço para o vetor de indices
        fread(fileSystem.indice, TAM_INDICE, 1, arquivo); //lê índices
        fileSystem.clusters = (CLUSTER*)(malloc(sizeof(CLUSTER)*TAM_INDICE)); //aloca espaço para o vetor de clusters
        //percorre todos os clusters, lendo seus metadados
        for (i = 0; i < TAM_INDICE; i++) {
            fread(&(fileSystem.clusters[i]), sizeof(CLUSTER), 1, arquivo); //lê meta do cluster
            fseek(arquivo, TAM_CLUSTER - sizeof(CLUSTER), SEEK_CUR); //pula pro próximo
        }
        fileSystem.arquivo = arquivo;
    }
    else {
        //se o arquivo não existe, cria o arquivo e escreve os dados padrão
        META_PROGRAMA meta = {TAM_INDICE, TAM_CLUSTER, I_INDICE, I_ROOT};
        char* indice = (char*)(malloc(sizeof(char)*TAM_INDICE)); //aloca espaço para o vetor de indices
        CLUSTER* clusters = (CLUSTER*)(malloc(sizeof(CLUSTER)*TAM_INDICE)); //aloca espaço para o vetor de clusters
        arquivo = fopen("data.bin", "wb+");
        fwrite(&meta, sizeof(meta), 1, arquivo); //escreve metadados
        char c = END_OF_FILE;
        fwrite(&c, sizeof(char), 1, arquivo); //escreve o root no indice
        indice[0] = c; //salva o valor pra ficar no FS
        c = VAZIO;
        for (i = 1; i < TAM_INDICE; i++) {
            fwrite(&c, sizeof(char), 1, arquivo); //escreve o resto do indice
            indice[i] = c; //salva o valor pra ficar no FS
        }
        CLUSTER cluster = {"root", "dir"};
        fwrite(&cluster, sizeof(CLUSTER), 1, arquivo); //escreve metadados do cluster root
        clusters[0] = cluster; //salva o valor pra ficar no FS
        c = END_OF_FILE;
        fwrite(&c, sizeof(char), 1, arquivo); //escreve end of file no inicio da area de dados do root
        c = VAZIO;
        for (i = 0; i < TAM_CLUSTER - sizeof(CLUSTER) - 1; i++) { //inicializa o cluster root com vazio
            fwrite(&c, sizeof(char), 1, arquivo);
        }
        for (i = 0; i < TAM_INDICE - 1; i++) { //inicializa o resto dos clusters
            initCluster(arquivo);
        }
        //salva dados pra retornar
        fileSystem.meta = meta;
        fileSystem.indice = indice;
        fileSystem.clusters = clusters;
        fileSystem.arquivo = arquivo;
    }
    //setta estado do diretório atual
    fileSystem.dirState.workingDirIndex = 0;
    strcpy(fileSystem.dirState.workingDir, "root");

    return fileSystem;
}

//Bruno
unsigned char getDirIndex(char* path, FS fileSystem) {}

//Bruno
void cd(char* path, FS fileSystem) {}

//Bruno
void dir(FS fileSystem) {}

//Arthur
void rm(char* path, FS fileSystem) {}

//Leo
void mkdir(char* name, FS fileSystem) {}

//Leo
void mkfile(char* name, FS fileSystem) {}

//Tiago
void edit(char* path, char* text, FS fileSystem) {}

//Arthur
void move(char* srcPath, char* destPath, FS fileSystem) {}

//Tiago
void renameFile(char* path, char* name, FS fileSystem) {}
