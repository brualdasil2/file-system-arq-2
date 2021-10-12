#include "fs.h"

//Escreve 32kb de VAZIO no arquivo, o equivalente a um cluster inteiro não inizializado (sem nome), com um END_OF_FILE no início da área de dados
void initCluster(FILE* arquivo) {
    int i;
    char c = '\0';
    for (i = 0; i < sizeof(CLUSTER); i++) { //metadados com 00
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
    if (arquivo = fileExists(NOME_ARQUIVO)) {
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
        arquivo = fopen(NOME_ARQUIVO, "wb+");
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
        for (i = sizeof(CLUSTER) + 1; i < TAM_CLUSTER; i++) { //inicializa o cluster root com vazio
            fwrite(&c, sizeof(char), 1, arquivo);
        }
        for (i = 1; i < TAM_INDICE; i++) { //inicializa o resto dos clusters
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

/*
==== FUNÇÕES UTILITÁRIAS ====
*/

//escreve todos os dados do FS no arquivo de memória
void saveFS(FS fileSystem) {
    int i;
    fseek(fileSystem.arquivo, 0, SEEK_SET); //vai pro inicio do arquivo
    fwrite(&(fileSystem.meta), sizeof(fileSystem.meta), 1, fileSystem.arquivo);
    fwrite(fileSystem.indice, TAM_INDICE, 1, fileSystem.arquivo);
    for (i = 0; i < TAM_INDICE; i++) {
        fwrite(&(fileSystem.clusters[i]), sizeof(CLUSTER), 1, fileSystem.arquivo); //escreve meta do cluster
        fseek(fileSystem.arquivo, TAM_CLUSTER - sizeof(CLUSTER), SEEK_CUR); //pula pro próximo
    }
}


void setPointerToCluster(FS fileSystem, unsigned char indice); //delcaração pra poder usar

//Retorna o índice do diretorio a partir do caminho, e VAZIO caso o caminho seja inválido
unsigned char getDirIndex(char* path, FS fileSystem) {
    char copiedPath[200]; //cópia da string (declarada localmente) pra funcionar no strtok
    strcpy(copiedPath, path);
    char *dirName = strtok(copiedPath, "/"); //armazena nome do diretório a ser encontrado
    if (strcmp(dirName, "root")) { //testa se o primeiro dir é o root
        return VAZIO;
    }
    dirName = strtok(NULL, "/"); //pega o nome do primeiro subdiretório
    unsigned char currDir = 0x00, c;
    int foundDir;
    while (dirName) { //percorre cada subdiretório do caminho
        setPointerToCluster(fileSystem, currDir);
        foundDir = 0;
        while(!foundDir) { //percorre cada byte da área de dados do diretório atual
            fread(&c, sizeof(unsigned char), 1, fileSystem.arquivo);
            if (c != END_OF_FILE) {
                if (!strcmp(dirName, fileSystem.clusters[c].nome)) { //verifica se é o nome do dir buscado, e se é do tipo dir
                    if (!strcmp("dir", fileSystem.clusters[c].tipo)) {
                        currDir = c;
                        foundDir = 1;
                    }
                }
            }
            else {
                return VAZIO; //se chegou ao fim e não encontrou, o caminho passado é inválido
            }
        }
        dirName = strtok(NULL, "/"); //só chega aqui se encontrou o dir buscado, salva dir seguinte a buscar
    }
    return currDir;
}

//Leo: posiciona o ponteiro do arquivo no início da área de dados do cluster
void setPointerToCluster(FS fileSystem, unsigned char indice) {
    fseek(fileSystem.arquivo, sizeof(fileSystem.meta) + TAM_INDICE + indice*TAM_CLUSTER + sizeof(CLUSTER), SEEK_SET);
}

//Leo: troca FE por itemIndex, e escreve FE logo dps
void appendItem(FS fileSystem, unsigned char dirIndex, unsigned char itemIndex) {}

//retorna o indice do primeiro cluster vazio na tabela
unsigned char findNextOpenCluster(FS fileSystem) {}

void OverWriteAt(FS fileSystem, char* text, unsigned char cIndex){//Função auxiliar OverWriteAt. Recebe o fileSystem, o texto que será inserido, e o índice da tabela atual. Recursiva.
  unsigned char nextClusterIndex;//Índice do próximo cluster.
  char* temp;//String temporária.
  char* extra;//String extra.
  int i;//Variável para laços.

  setPointerToCluster(fileSystem,cIndex);//Aponta o sistema de escrita para o cluster do índice.
  temp = (char*)malloc(MAX_CHAR*sizeof(char));//Define a string temporária.


  if(strlen(text) < MAX_CHAR){//Caso o texto seja menor que a maior quantidade de caracteres do cluster,
      fwrite(text, strlen(text)+1, 1, fileSystem.arquivo);//Escreve o texto no cluster.
  }else{//Se não, executa:
      for(i=0;i<MAX_CHAR;i++){//Salva a parte que será salva no cluster atual em temp.
          *(temp+i) = *(text+i);
      }
      *(temp+MAX_CHAR) = '\0';//Termina a escrita da string.

      extra = (char*)malloc(sizeof(text)*sizeof(char));//Define a string extra.

      for(i=MAX_CHAR;i<strlen(text)+1;i++){//Salva o que restou da string em extra.
          *(extra+i-MAX_CHAR) = *(text+i);
      }

      fwrite(temp, sizeof(temp), 1, fileSystem.arquivo);//Escreve o texto no cluster atual.

      //nextClusterIndex = findNextOpenCluster(fileSystem);
      nextClusterIndex = 1;
      fileSystem.indice[cIndex] = nextClusterIndex;//Redefine a tabela atual do cluster para o próximo cluster.
      fileSystem.indice[nextClusterIndex] = END_OF_FILE;//Define o próximo como END_OF_FILE.
      OverWriteAt(fileSystem,extra,nextClusterIndex);//Recursivamente, escreve no próximo cluster.
  }
}


/*
==== FUNÇÕES DE COMANDOS ====
*/


//Altera o diretório atual a partir do caminho, se ele existir
void cd(char* path, FS* fileSystem) {
    unsigned char currDir = getDirIndex(path, *fileSystem);
    if (currDir == VAZIO) {
        printf("Esse diretorio nao existe\n");
    }
    else {
        fileSystem->dirState.workingDirIndex = currDir;
        strcpy(fileSystem->dirState.workingDir, path);
    }
}

//Bruno
void dir(FS fileSystem) {}

//Arthur
void rm(char* path, FS fileSystem) {}

//Leo
void mkdir(char* name, FS fileSystem) {}

//Leo
void mkfile(char* name, FS fileSystem) {}

//Tiago
void edit(char* path, char* text, FS fileSystem) {//Função edit. Executa o comando EDIT. Recebe o caminho do arquivo, o texto para inserir, fileSystem.
    unsigned char cIndex;//Comentário linha 240.

    cIndex = getDirIndex(path,fileSystem);//Define o índice do caminho entregue.

    if(cIndex == VAZIO){//Caso o diretório não exista, executa:
        printf("Esse diretorio nao existe\n");
    }else{//Se não, caso normal:
        OverWriteAt(fileSystem,text,cIndex);//Executa função auxiliar de escrita.
        saveFS(fileSystem);//Salva o arquivo.
    }
}

//Arthur
void move(char* srcPath, char* destPath, FS fileSystem) {}

//Tiago
void renameFile(char* path, char* name, FS fileSystem) {//Função renameFile. Executa o comando RENAME. Recebe o caminho do arquivo, o nome novo e o fileSystem.
    unsigned char cIndex;//Índice atual. Do inglês, current index.

    cIndex = getDirIndex(path,fileSystem);//Define o índice do caminho entregue.

    if(cIndex == VAZIO){//Caso o diretório não exista, executa:
        printf("Esse diretorio nao existe\n");//Prompt de erro em caso de diretório incorreto.
    }else{//Se não, caso normal:
        if(!(strcmp(name,'\0')==0)){
            strcpy(fileSystem.clusters[cIndex].nome,name);//Executa a troca de nome.
            saveFS(fileSystem);//Salva o arquivo.
        }else{
            printf("Nome inválido para renomear. Detalhe: nome vazio.\n");//Prompt de erro em caso de diretório incorreto.
        }
    }
}
