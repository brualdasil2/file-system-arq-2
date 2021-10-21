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

void setPointerToCluster(FS fileSystem, unsigned char indice); //declaração pra poder usar

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
void appendItem(FS fileSystem, unsigned char dirIndex, unsigned char itemIndex) {
    char item = CORROMPIDO;
    char auxChar;
    setPointerToCluster(fileSystem, dirIndex); // coloca o pointer no cluster
    // acha o fim do diretorio
    while((unsigned char)item != END_OF_FILE){
        if ((unsigned char)item == VAZIO){
            fseek(fileSystem.arquivo, -1*sizeof(char), SEEK_CUR);
            auxChar = itemIndex;
            fwrite(&auxChar, sizeof(char), 1, fileSystem.arquivo);
            return;
        }
        fread(&item, sizeof(char), 1, fileSystem.arquivo);
    }
    //coloca mais um indice(dirIndex) e poe o FE
    fseek(fileSystem.arquivo, -1*sizeof(char), SEEK_CUR);
    auxChar = itemIndex;
    fwrite(&auxChar, sizeof(char), 1, fileSystem.arquivo);
    auxChar = END_OF_FILE;
    fwrite(&auxChar, sizeof(char), 1, fileSystem.arquivo);
}

//retorna o indice do primeiro cluster vazio na tabela
unsigned char findNextOpenCluster(FS fileSystem) {
    int i = 0;
    while(i<TAM_INDICE){
        if((unsigned char)fileSystem.indice[i] == VAZIO){
            return i;
        }
        i++;
    }
    return CORROMPIDO;
}

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

      nextClusterIndex = findNextOpenCluster(fileSystem);//Busca o índice do próximo cluster.
      fileSystem.indice[cIndex] = nextClusterIndex;//Redefine a tabela atual do cluster para o próximo cluster.
      fileSystem.indice[nextClusterIndex] = END_OF_FILE;//Define o próximo como END_OF_FILE.
      OverWriteAt(fileSystem,extra,nextClusterIndex);//Recursivamente, escreve no próximo cluster.
  }
}

//Arthur: Testa se o diretório está vazio.
int dirIsEmpty(unsigned char dirIndex, FS fileSystem) {
    unsigned char c;
    setPointerToCluster(fileSystem, dirIndex);
    while(1) {
        fread(&c, sizeof(unsigned char), 1, fileSystem.arquivo);
        if (c == END_OF_FILE) return 1;
        else if (c != VAZIO) return 0;
    }
}
//Arthur: Procura no diretório indicado um arquivo com nome e tipo específico.
unsigned char isInDir(unsigned char dirIndex, char* archiveName, char* archiveType, FS fileSystem) {
    unsigned char c = VAZIO;
    setPointerToCluster(fileSystem, dirIndex);      
    while(c!=END_OF_FILE) {                  
        fread(&c, sizeof(unsigned char), 1, fileSystem.arquivo);        
        if (!strcmp(archiveName, fileSystem.clusters[c].nome) && !strcmp(archiveType, fileSystem.clusters[c].tipo)) {
            return c;
        }           
    }
    return VAZIO;
}
//Arthur: Guarda o valor dos indíces do arquivo inferior(último no path) e arquivo superior(penúltimo no path), altera para VAZIO se inválido.
void getLastTwoIndex(char* path,  unsigned char* upperArchiveIndex, unsigned char* lowerArchiveIndex, FS fileSystem) {
    char* breakPoint;
    char* lowerArchiveName;
    char lowerArchiveType[4];
    if(path == NULL) return;
    char upperPath[strlen(path)];

    strcpy(upperPath,path);
    breakPoint = strrchr(upperPath, '/');
    if (breakPoint != NULL) {
        breakPoint[0] = '\0';
        lowerArchiveName = breakPoint + 1;
    }

    if (upperPath[0] != '\0') //getDirIndex da crash se receber '\0'
        *upperArchiveIndex = getDirIndex(upperPath, fileSystem);

    *lowerArchiveIndex = (unsigned char)VAZIO;

    if (lowerArchiveName[strlen(lowerArchiveName) - 4] == '.') {
        strcpy(lowerArchiveType, lowerArchiveName + strlen(lowerArchiveName) - 3);
        lowerArchiveName[strlen(lowerArchiveName) - 4] = '\0';
    }
    else strcpy(lowerArchiveType, "dir");
    if (breakPoint != NULL && upperPath[0] != '\0' && lowerArchiveName[0] != '\0')
        *lowerArchiveIndex = isInDir(*upperArchiveIndex,lowerArchiveName, lowerArchiveType, fileSystem);
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

//Lista os arquivos e diretórios do diretório atual
void dir(FS fileSystem) {
    setPointerToCluster(fileSystem, fileSystem.dirState.workingDirIndex); //vai pro dir atual
    unsigned char c;
    int fileCounter = 0;
    do { //lê cada índice do dir
        fread(&c, sizeof(unsigned char), 1, fileSystem.arquivo);
        if (c != END_OF_FILE && c != VAZIO && c != CORROMPIDO) { //se for um indice válido
            if ((unsigned char)fileSystem.indice[c] != VAZIO) { //consulta a tabela de indices pra ver se o arquivo ainda existe mesmo
                //printa o nome do arquivo ou diretorio com formatação
                if (!strcmp(fileSystem.clusters[c].tipo, "dir")) {
                    printf("<DIR>");
                }
                printf("\t%s", fileSystem.clusters[c].nome);
                if (strcmp(fileSystem.clusters[c].tipo, "dir")) {
                    printf(".%s", fileSystem.clusters[c].tipo);
                }
                printf("\n");
                fileCounter++;
            }
        }
    } while (c != END_OF_FILE);
    if (fileCounter == 0) { //se não tinha nenhum arquivo
        printf("\t<VAZIO>\n");
    }
}

//Arthur: Faz a remoção de um arquivo.
void rm(char* path, FS* fileSystem) {
    unsigned char upper,lower;
    upper = lower = VAZIO;

    getLastTwoIndex(path, &upper, &lower, *fileSystem);
    if((upper != VAZIO && lower != VAZIO) && (!(strcmp("dir",fileSystem->clusters[lower].tipo) == 0) || dirIsEmpty(lower, *fileSystem))){
        isInDir(upper, fileSystem->clusters[lower].nome, fileSystem->clusters[lower].tipo, *fileSystem);
        fseek(fileSystem->arquivo, (long int)(-1*sizeof(char)), SEEK_CUR);
        putc(VAZIO,fileSystem->arquivo);

        if(fileSystem->dirState.workingDirIndex == lower)  {
            fileSystem->dirState.workingDirIndex = 0;
            strcpy(fileSystem->dirState.workingDir, "root");
        }
        do {
            upper = fileSystem->indice[lower]; //Reaproveita upper para guardar o apontador localizado na posição lower da tabela de índices.
            fileSystem->indice[lower] = VAZIO;
            lower = upper;
        } while (lower != END_OF_FILE); //Certifica-se de excluir todos os clusters referentes ao arquivo.
        saveFS(*fileSystem);
    }
    else printf("Caminho Invalido.\n");
}

//Leo

void separatePaths(char* fullPath, char* path, char* itemName){
    int i, j, lastBarIndex;
    i = j = lastBarIndex = 0;
    while (fullPath[i] != '\0'){
            if(fullPath[i]=='/'){
                lastBarIndex = i;
            }
            i++;
        }
        i=0;
        while(i<lastBarIndex){
            path[i]=fullPath[i];
            i++;
        }
        i++;
        while(fullPath[i]!= '\0'){
            itemName[j] = fullPath[i];
            i++;
            j++;
        }
}

//Leo
void make(char* name, char* type, FS fileSystem) {
    char itemName[200] = "";    //item inserido
    char path[200] = "";        //caminho ate o item
    // completa as strings acima
    if (name[0] != '/'){ // mkfile file.txt -> cria no diretório atual.
        strcpy(path, fileSystem.dirState.workingDir);
        strcpy(itemName, name);
    }else{ // caso /root/dir/file.txt -> cria no caminho especificado.
        separatePaths(name, path,itemName); 
    }
    // Consistencia -> caminho valido ? Arquivo ja existe ? Tamanho do nome ?
    unsigned char clusterOfDirIndex = getDirIndex(path, fileSystem);
    if (clusterOfDirIndex == VAZIO){
        printf("Caminho Invalido\n");
        return;
    }
    if( isInDir(clusterOfDirIndex, itemName, type, fileSystem) != VAZIO){
        printf("Arquivo ou diretorio ja existe\n");
        return;
    }
    if(strlen(itemName)>19){
        printf("Abortado! Nome muito grande");
    }
    // Altera o indice
    unsigned char clusterIndex = findNextOpenCluster(fileSystem);
    fileSystem.indice[clusterIndex] = END_OF_FILE;
    // Altera o diretorio
    appendItem(fileSystem, clusterOfDirIndex, clusterIndex);
    // Salva o FS
    strcpy(fileSystem.clusters[clusterIndex].nome, itemName);
    strcpy(fileSystem.clusters[clusterIndex].tipo, type);
    saveFS(fileSystem);
}

//Tiago
void edit(char* path, char* text, FS fileSystem) {//Função edit. Executa o comando EDIT. Recebe o caminho do arquivo, o texto para inserir, fileSystem.
    unsigned char originUpper, originLower;

    getLastTwoIndex(path, &originUpper, &originLower, fileSystem);//Recupera o índice do arquivo e do diretório.

    if(originLower == VAZIO){//Caso o arquivo não exista, executa:
        printf("Esse diretorio nao existe\n");
    }else{//Se não, caso normal:
        if(strcmp(fileSystem.clusters[originLower].tipo,"txt")==0){
            OverWriteAt(fileSystem,text,originLower);//Executa função auxiliar de escrita.
            saveFS(fileSystem);//Salva o arquivo.
        }else{
            printf("Arquivo inválido para edição.\n");
        }        
    }
}

//Arthur: Move um arquivo para uma pasta designada
void move(char* srcPath, char* destPath, FS* fileSystem) {
    unsigned char originUpper, originLower, destLower;
    originUpper = originLower = destLower = VAZIO;
    
    if (destPath != NULL && destPath[0] != '\0') destLower = getDirIndex(destPath, *fileSystem); //getDirIndex da crash se receber '\0'
    getLastTwoIndex(srcPath, &originUpper, &originLower, *fileSystem);

    if(originUpper != VAZIO && originLower != VAZIO && destLower != VAZIO && (originLower != destLower)){
        if(isInDir(destLower, fileSystem->clusters[originLower].nome, fileSystem->clusters[originLower].tipo, *fileSystem) == VAZIO) {
            isInDir(originUpper, fileSystem->clusters[originLower].nome, fileSystem->clusters[originLower].tipo, *fileSystem);
            fseek(fileSystem->arquivo, (long int)(-1*sizeof(char)), SEEK_CUR);
            putc(VAZIO,fileSystem->arquivo);
            appendItem(*fileSystem, destLower, originLower);

            if(fileSystem->dirState.workingDirIndex == originLower)  {
                fileSystem->dirState.workingDirIndex = 0;
                strcpy(fileSystem->dirState.workingDir, "root");
            }
        }
        else printf("O diretorio de destino ja contem um arquivo com o mesmo nome e tipo.\n");
    }
    else printf("Caminhos Invalidos.\n");
}

//Tiago
void renameFile(char* path, char* name, FS fileSystem) {//Função renameFile. Executa o comando RENAME. Recebe o caminho do arquivo, o nome novo e o fileSystem.
    unsigned char originUpper, originLower;

    getLastTwoIndex(path, &originUpper, &originLower, fileSystem);
    
    if(originUpper == VAZIO){//Caso o diretório não exista, executa:
        printf("Esse diretorio nao existe\n");//Prompt de erro em caso de diretório incorreto.
    }else{//Se não, caso normal:
        if(originLower == VAZIO){            
            printf("Arquivo inválido.\n");//Prompt de erro em caso de arquivo inválido.           
        }else{//Se não, caso normal:  
            if(isInDir(originUpper, name, fileSystem.clusters[originLower].tipo, fileSystem)==VAZIO){
                strcpy(fileSystem.clusters[originLower].nome,name);//Executa a troca de nome.
                saveFS(fileSystem);//Salva o arquivo.
            }else{
                printf("Nome inválido para renomear. Detalhe: nome já utilizado.\n");//Prompt de erro em caso de nome inválido.
            } 
        }
    }
}
