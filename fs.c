#include "fs.h"


/*
=============== FUNÇÕES UTILITÁRIAS ===============
*/

//posiciona o ponteiro do arquivo no início da área de dados do cluster
void setPointerToCluster(FS fileSystem, unsigned char indice) {
    fseek(fileSystem.arquivo, sizeof(fileSystem.meta) + fileSystem.meta.tam_indice + indice*fileSystem.meta.tam_cluster + sizeof(CLUSTER), SEEK_SET);
}

//escreve todos os dados do FS no arquivo de memória
void saveFS(FS fileSystem) {
    int i;
    fseek(fileSystem.arquivo, 0, SEEK_SET); //vai pro inicio do arquivo
    fwrite(&(fileSystem.meta), sizeof(fileSystem.meta), 1, fileSystem.arquivo);
    fwrite(fileSystem.indice, fileSystem.meta.tam_indice, 1, fileSystem.arquivo);
    for (i = 0; i < fileSystem.meta.tam_indice; i++) {
        fwrite(&(fileSystem.clusters[i]), sizeof(CLUSTER), 1, fileSystem.arquivo); //escreve meta do cluster
        fseek(fileSystem.arquivo, fileSystem.meta.tam_cluster - sizeof(CLUSTER), SEEK_CUR); //pula pro próximo
    }
}

//Escreve 32kb de VAZIO no arquivo, o equivalente a um cluster inteiro não inizializado (sem nome), com um END_OF_FILE no início da área de dados
void initCluster(FILE* arquivo, unsigned short tam_cluster) {
    int i;
    char c = '\0';
    for (i = 0; i < sizeof(CLUSTER); i++) { //metadados com 00
        fwrite(&c, sizeof(char), 1, arquivo);
    }
    c = END_OF_FILE;
    fwrite(&c, sizeof(char), 1, arquivo); //END_OF_FILE pra marcar o início da área de dados
    c = VAZIO;
    for (i = 0; i < tam_cluster - sizeof(CLUSTER) - 1; i++) { //área de dados com VAZIO
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

//Retorna o índice do diretorio a partir do caminho, e VAZIO caso o caminho seja inválido
unsigned char getDirIndex(char* path, FS fileSystem) {
    char copiedPath[MAX_PATHNAME_SIZE]; //cópia da string (declarada localmente) pra funcionar no strtok
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

//troca FE por itemIndex, e escreve FE logo dps
void appendItem(FS fileSystem, unsigned char dirIndex, unsigned char itemIndex) {
    char item = CORROMPIDO;
    char auxChar;
    setPointerToCluster(fileSystem, dirIndex); // coloca o pointer no cluster
    // acha o fim do diretorio
    while((unsigned char)item != END_OF_FILE){
        if ((unsigned char)item == VAZIO){
            fseek(fileSystem.arquivo, (long int)(-1*sizeof(char)), SEEK_CUR);
            auxChar = itemIndex;
            fwrite(&auxChar, sizeof(char), 1, fileSystem.arquivo);
            return;
        }
        fread(&item, sizeof(char), 1, fileSystem.arquivo);
    }
    //coloca mais um indice(dirIndex) e poe o FE
    fseek(fileSystem.arquivo, (long int)(-1*sizeof(char)), SEEK_CUR);
    auxChar = itemIndex;
    fwrite(&auxChar, sizeof(char), 1, fileSystem.arquivo);
    auxChar = END_OF_FILE;
    fwrite(&auxChar, sizeof(char), 1, fileSystem.arquivo);
}

//retorna o indice do primeiro cluster vazio na tabela
unsigned char findNextOpenCluster(FS fileSystem) {
    int i = 0;

    while(i<fileSystem.meta.tam_indice-3){
        if((unsigned char)fileSystem.indice[i] == VAZIO){
            return i;
        }
        i++;
    }
    // se esta cheio retorna corrompido
    return CORROMPIDO;
}

//retorna o indice do ultimo cluster ocupado na tabela
unsigned char findLastUsedCluster(FS fileSystem) {
    unsigned short i = fileSystem.meta.tam_indice - 3 - 1;

    while(i >= 0) {
        if((unsigned char)fileSystem.indice[i] != VAZIO){
            return i;
        }
        i--;
    }
    // se tudo incluindo root vazio retorna corrompido
    return CORROMPIDO;
}

//Função auxiliar OverWriteAt. Recebe o fileSystem, o texto que será inserido, e o índice da tabela atual. Recursiva.
void OverWriteAt(FS* fileSystem, char* text, unsigned char cIndex){
  unsigned char nextClusterIndex;//Índice do próximo cluster.
  char* temp;//String temporária.
  char* extra;//String extra.
  int i;//Variável para laços. 

  setPointerToCluster(*fileSystem,cIndex);//Aponta o sistema de escrita para o cluster do índice.
  temp = (char*)malloc(MAX_CHAR*sizeof(char));//Define a string temporária.

  if(strlen(text) < MAX_CHAR){//Caso o texto seja menor que a maior quantidade de caracteres do cluster,    
      fwrite(text, strlen(text)+1, 1, fileSystem->arquivo);//Escreve o texto no cluster.
  }else{//Se não, executa:
      for(i=0;i<MAX_CHAR;i++){//Salva a parte que será salva no cluster atual em temp.
          *(temp+i) = *(text+i);
      }
      *(temp+MAX_CHAR) = '\0';//Termina a escrita da string.

      extra = (char*)malloc(strlen(text)*sizeof(char));//Define a string extra.

      for(i=MAX_CHAR;i<strlen(text)+1;i++){//Salva o que restou da string em extra.
          *(extra+i-MAX_CHAR) = *(text+i);
      }

      fwrite(temp, strlen(temp)+1, 1, fileSystem->arquivo);//Escreve o texto no cluster atual.

      nextClusterIndex = findNextOpenCluster(*fileSystem);//Busca o índice do próximo cluster.
      fileSystem->indice[cIndex] = nextClusterIndex;//Redefine a tabela atual do cluster para o próximo cluster.
      fileSystem->indice[nextClusterIndex] = END_OF_FILE;//Define o próximo como END_OF_FILE.
      OverWriteAt(fileSystem,extra,nextClusterIndex);//Recursivamente, escreve no próximo cluster.
      free(extra);
    }
    free(temp);    
}

//Testa se o diretório está vazio.
int dirIsEmpty(unsigned char dirIndex, FS fileSystem) {
    unsigned char c;
    setPointerToCluster(fileSystem, dirIndex);
    while(1) {
        fread(&c, sizeof(unsigned char), 1, fileSystem.arquivo);
        if (c == END_OF_FILE) return 1;
        else if (c != VAZIO) return 0;
    }
}

//Procura no diretório indicado um arquivo com nome e tipo específico.
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

//Separa o nome do arquivo do resto do caminho
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
    path[i] = '\0';
    i++;
    while(fullPath[i]!= '\0'){
        itemName[j] = fullPath[i];
        i++;
        j++;
    }
    itemName[j] = '\0';
}

//Separa o nome do arquivo da extensão
unsigned char separateFileNameAndType(char* fullName, char** fileName, char** fileType) {
	 if(fullName == NULL) return 0;	
    int nameSize = strlen(fullName);
    if (fullName[nameSize - EXTENSION_SIZE] != '.') {
        return 0;
    }
    else {
        *fileName = strtok(fullName, ".");
        *fileType = strtok(NULL, ".");
        return 1;
    }
}

// se o nome for valido, retorna -1 (invalido), se nao tem retorna 1 (valido)
int validateName(char* name){
	 if (name == NULL) return -1;
	 int i = strlen(name);
    if (name[i-EXTENSION_SIZE] == '.'){ //Caso o usuário insira um nome com fim .txt
        name[i-EXTENSION_SIZE] = '\0';
    }
    if (strlen(name)>MAX_FILENAME_SIZE-1) return-1;
    for(int i=0 ; i<strlen(name) ; i++){
        if (name[i] == '/'){
            return -1;
        }
    }
    return 1;
}

//Guarda o valor dos indíces do arquivo inferior(último no path) e arquivo superior(penúltimo no path), altera para VAZIO se inválido.
void getLastTwoIndex(char* path,  unsigned char* upperArchiveIndex, unsigned char* lowerArchiveIndex, FS fileSystem) {
    char* breakPoint;
    char* lowerArchiveName;
    char lowerArchiveType[EXTENSION_SIZE] = "dir";
    char upperPath[MAX_PATHNAME_SIZE];

    if(path == NULL) return;

    strcpy(upperPath,path);
    breakPoint = strrchr(upperPath, '/');
    if (breakPoint != NULL) {
        breakPoint[0] = '\0';
        lowerArchiveName = breakPoint + 1;

        if (lowerArchiveName[strlen(lowerArchiveName) - EXTENSION_SIZE] == '.') {
            strcpy(lowerArchiveType, lowerArchiveName + strlen(lowerArchiveName) - 3);
            lowerArchiveName[strlen(lowerArchiveName) - EXTENSION_SIZE] = '\0';
        }
    }

    if (upperPath[0] != '\0') //getDirIndex da crash se receber '\0'
        *upperArchiveIndex = getDirIndex(upperPath, fileSystem);

    *lowerArchiveIndex = (unsigned char)VAZIO;

    if (breakPoint != NULL && upperPath[0] != '\0' && lowerArchiveName[0] != '\0')
        *lowerArchiveIndex = isInDir(*upperArchiveIndex,lowerArchiveName, lowerArchiveType, fileSystem);
}

//função recursiva: retorna o tamanho do diretório e de seus sub-diretórios
int getDirSize(FS fileSystem, unsigned char dir){
    unsigned char itemfromDir = VAZIO;  // item que sera inspecionado
    int dirSize = 1;                    // armazena o tamanho ( vale 1 porque o diretorio 1 quando vazio)

    setPointerToCluster(fileSystem, dir);
    // le 1 item -> podem ser 4 casos: (1) arquivo > 1 cluster / (2) diretorio / (3)arquivo de 1 cluster / (4) fim do dir
    fread(&itemfromDir, sizeof(char), 1, fileSystem.arquivo);
    while(itemfromDir != END_OF_FILE){
        if(itemfromDir != VAZIO){// so executa o codigo abaixo se o item do dir nao for vazio
            // (1) -> percorre o indice ate achar o fim do arquivo (incrementando o tamanho do diretorio)
            if((unsigned char)fileSystem.indice[itemfromDir] != END_OF_FILE){
                unsigned char aux = itemfromDir;    // variavel auxiliar, para nao perder o valor do itemFromDir
                while(aux!= END_OF_FILE && aux != VAZIO){
                    aux = fileSystem.indice[aux];
                    dirSize++;
            }
            // (2) -> salva o estado do filePointer e chama essa funcao recursivamente para o diretorio       
            }else if(strcmp(fileSystem.clusters[itemfromDir].tipo, "dir")==0){
                long state = ftell(fileSystem.arquivo); // variavel que salve o estado do filePointer
                dirSize += getDirSize(fileSystem, itemfromDir);
                fseek(fileSystem.arquivo, state, SEEK_SET);
            }
            // (3) -> somente incrementa o tamanho do diretorio
            else{
                dirSize++;
            }
        }
        fread(&itemfromDir, sizeof(char), 1, fileSystem.arquivo);
    }
    return dirSize; 
}

//Testa se um índice está em um diretório
int isIndexInDir(FS fileSystem, unsigned char index, unsigned char dirIndex) {
    unsigned char c;
    setPointerToCluster(fileSystem, dirIndex);
    do {
        fread(&c, sizeof(unsigned char), 1, fileSystem.arquivo);
        if (index == c) {
            return 1;
        }
    } while (c != END_OF_FILE);
    return 0;
}

//retorna o índice do diretório pai do cluster. Caso não encontre (ex: cluster que seja parte de um arquivo multi-cluster), retorna CORROMPIDO
unsigned char findParentDirIndex(FS fileSystem, unsigned char index) {
    int i;
    //percorre a tabela de indices
    for (i = 0; i < fileSystem.meta.tam_indice - UNUSED_DIRS; i++) {
        if (!strcmp(fileSystem.clusters[i].tipo, "dir")) {
            //pra cada posição que é um dir, testa se esse indice está nele
            if (isIndexInDir(fileSystem, index, i)) {
                return i;
            }
        }
    }
    return CORROMPIDO;
}

//move o conteúdo todo de um cluster para outra posição, sobrescrevendo o destino
void moveCluster(FS* fileSystem, unsigned char srcIndex, unsigned char destIndex) {
    //lê dados do cluster
    char* clusterData = (char*)(malloc(fileSystem->meta.tam_cluster));
    setPointerToCluster(*fileSystem, srcIndex);
    fread(clusterData, fileSystem->meta.tam_cluster, 1, fileSystem->arquivo);
    //escreve metadados do cluster na pos nova
    strcpy(fileSystem->clusters[destIndex].nome, fileSystem->clusters[srcIndex].nome);
    strcpy(fileSystem->clusters[destIndex].tipo, fileSystem->clusters[srcIndex].tipo);
    //escreve dados do cluster na pos nova
    setPointerToCluster(*fileSystem, destIndex);
    fwrite(clusterData, fileSystem->meta.tam_cluster, 1, fileSystem->arquivo);
    //escreve o valor do indice na pos nova da tabela
    fileSystem->indice[destIndex] = fileSystem->indice[srcIndex];
    //escreve VAZIO na pos antiga da tabela
    fileSystem->indice[srcIndex] = VAZIO;

    //encontra indice do dir que aponta pro indice antigo desse cluster
    unsigned char parentIndex = findParentDirIndex(*fileSystem, srcIndex);
    if (parentIndex != CORROMPIDO) {
        //troca esse valor na área de dados do dir pai pelo indice novo
        setPointerToCluster(*fileSystem, parentIndex);
        unsigned char c;
        do {
            fread(&c, sizeof(unsigned char), 1, fileSystem->arquivo);
        } while (c != srcIndex);
        fseek(fileSystem->arquivo, -1, SEEK_CUR);
        fwrite(&destIndex, sizeof(unsigned char), 1, fileSystem->arquivo);
    }
    //se for um pedaço de arquivo grande...
    else {
        //busca qual outro arquivo de texto aponta pro indice antigo
        for (int i = 0; i < fileSystem->meta.tam_indice - 3; i++) {
            if (fileSystem->indice[i] == srcIndex) {
                //troca o valor do indice desse arquivo pelo indice novo
                fileSystem->indice[i] = destIndex;
            }
        }
    }

    saveFS(*fileSystem);
}

//cria um arquivo ou diretório
void make(char* name, char* type, FS* fileSystem) {
    // Consistencia -> Nome valido? Arquivo ja existe ? Armazenamento está cheio ?
    if (validateName(name) == -1 ){
        printf("Nome invalido!\n");
        return;
    }
    if( isInDir(fileSystem->dirState.workingDirIndex, name, type, *fileSystem) != VAZIO){
        printf("Arquivo ou diretorio ja existe\n");
        return;
    }
    unsigned char clusterIndex = findNextOpenCluster(*fileSystem);
    if (clusterIndex == CORROMPIDO){
        printf("Seu Armazenamento esta cheio!\n");
        return;
    }

    // Altera o indice
    fileSystem->indice[clusterIndex] = END_OF_FILE;
    // Altera o diretorio
    appendItem(*fileSystem, fileSystem->dirState.workingDirIndex, clusterIndex);
    // Salva o FS
    strcpy(fileSystem->clusters[clusterIndex].nome, name);
    strcpy(fileSystem->clusters[clusterIndex].tipo, type);
    saveFS(*fileSystem);
}

/*
=============== FUNÇÕES DE COMANDOS ===============
*/

//Inicializa o arquivo e as estruturas do FS
FS initFS() {
    FILE* arquivo;
    FS fileSystem;
    int i;
    if (arquivo = fileExists(NOME_ARQUIVO)) {
        //se o arquivo já existe, só lê os dados e salva no FS
        fread(&(fileSystem.meta), sizeof(fileSystem.meta), 1, arquivo); //lê metadados
        fileSystem.indice = (char*)(malloc(sizeof(char)*fileSystem.meta.tam_indice)); //aloca espaço para o vetor de indices
        fread(fileSystem.indice, fileSystem.meta.tam_indice, 1, arquivo); //lê índices
        fileSystem.clusters = (CLUSTER*)(malloc(sizeof(CLUSTER)*fileSystem.meta.tam_indice)); //aloca espaço para o vetor de clusters
        //percorre todos os clusters, lendo seus metadados
        for (i = 0; i < fileSystem.meta.tam_indice; i++) {
            fread(&(fileSystem.clusters[i]), sizeof(CLUSTER), 1, arquivo); //lê meta do cluster
            fseek(arquivo, fileSystem.meta.tam_cluster - sizeof(CLUSTER), SEEK_CUR); //pula pro próximo
        }
        fileSystem.arquivo = arquivo;
    }
    else {
        //se o arquivo não existe, cria o arquivo e escreve os dados padrão
        META_PROGRAMA meta = {TAM_INDICE, TAM_CLUSTER, I_INDICE, I_ROOT};
        char* indice = (char*)(malloc(sizeof(char)*meta.tam_indice)); //aloca espaço para o vetor de indices
        CLUSTER* clusters = (CLUSTER*)(malloc(sizeof(CLUSTER)*meta.tam_indice)); //aloca espaço para o vetor de clusters
        arquivo = fopen(NOME_ARQUIVO, "wb+");
        fwrite(&meta, sizeof(meta), 1, arquivo); //escreve metadados
        char c = END_OF_FILE;
        fwrite(&c, sizeof(char), 1, arquivo); //escreve o root no indice
        indice[0] = c; //salva o valor pra ficar no FS
        c = VAZIO;
        for (i = 1; i < meta.tam_indice; i++) {
            fwrite(&c, sizeof(char), 1, arquivo); //escreve o resto do indice
            indice[i] = c; //salva o valor pra ficar no FS
        }
        CLUSTER cluster = {"root", "dir"};
        fwrite(&cluster, sizeof(CLUSTER), 1, arquivo); //escreve metadados do cluster root
        clusters[0] = cluster; //salva o valor pra ficar no FS
        c = END_OF_FILE;
        fwrite(&c, sizeof(char), 1, arquivo); //escreve end of file no inicio da area de dados do root
        c = VAZIO;
        for (i = sizeof(CLUSTER) + 1; i < meta.tam_cluster; i++) { //inicializa o cluster root com vazio
            fwrite(&c, sizeof(char), 1, arquivo);
        }
        for (i = 1; i < meta.tam_indice; i++) { //inicializa o resto dos clusters
            initCluster(arquivo, meta.tam_cluster);
        }
        //salva dados pra retornar
        fileSystem.meta = meta;
        fileSystem.indice = indice;
        fileSystem.clusters = clusters;
        fileSystem.arquivo = arquivo;
    }
    //setta estado do diretório atual
    fileSystem.dirState.workingDirIndex = 0;
    strcpy(fileSystem.dirState.workingDir, "/root");
    
    return fileSystem;
}

//Finaliza o sistema de arquivos
int closeFS(FILE* fileP, char* indexPointer, CLUSTER* clustersP){ // Função closeFS. Realiza processo de encerramento. 
    fclose(fileP);//Encerra o stream do arquivo.
    free(indexPointer);//Libera os ponteiros.
    free(clustersP);
    return 0;
}

//Altera o diretório atual a partir do caminho, se ele existir
void cd(char* path, FS* fileSystem) {
    if (path == NULL) printf("Caminho Invalido.\n");
    else {
        unsigned char currDir = getDirIndex(path, *fileSystem);
        if (currDir == VAZIO) {
            printf("Esse diretorio nao existe\n");
        }
        else {
            fileSystem->dirState.workingDirIndex = currDir;
            strcpy(fileSystem->dirState.workingDir, path);
        }
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

//Faz a remoção de um arquivo.
void rm(char* path, FS* fileSystem) {
    unsigned char upper,lower;
    upper = lower = VAZIO;

    getLastTwoIndex(path, &upper, &lower, *fileSystem);
    if (upper != VAZIO && lower != VAZIO){ //Testa se os caminhos estão corretos.
        //Testa, caso for um diretório, se ele está vazio.
        if (!(strcmp("dir",fileSystem->clusters[lower].tipo) == 0) || dirIsEmpty(lower, *fileSystem)) {
            isInDir(upper, fileSystem->clusters[lower].nome, fileSystem->clusters[lower].tipo, *fileSystem);
            fseek(fileSystem->arquivo, (long int)(-1*sizeof(char)), SEEK_CUR);
            putc(VAZIO,fileSystem->arquivo);

            if (fileSystem->dirState.workingDirIndex == lower)  {
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
        else printf("O diretorio precisa estar vazio.\n");
    }
    else printf("Caminho Invalido.\n");
}

//Cria um dir no dir atual
void mkdir(char* name, FS* fileSystem) {
    make(name, "dir", fileSystem);
}

//Cria um arquivo no dir atual
void mkfile(char* name, FS* fileSystem) {
    char* path;
    char* fileType;

    if (separateFileNameAndType(name, &path, &fileType)) {
        make(path, fileType, fileSystem);
    }
    else {
        printf("Nome invalido!\n");
    }
}

//Escreve conteúdo de texto nos dados de um arquivo
void edit(char* path, char* text, FS* fileSystem) {//Função edit. Executa o comando EDIT. Recebe o caminho do arquivo, o texto para inserir, fileSystem.
    unsigned char originUpper, originLower;
    
    getLastTwoIndex(path, &originUpper, &originLower, *fileSystem);//Recupera o índice do arquivo e do diretório.
    
    if (text == NULL || *text == '\0') printf("Termos invalidos\n");
    else if (originUpper == VAZIO || originLower == VAZIO){//Caso o arquivo não exista, executa:
        printf("Esse diretorio nao existe\n");
    }
    else{//Se não, caso normal:
        if (strcmp(fileSystem->clusters[originLower].tipo,"txt")==0){
            OverWriteAt(fileSystem,text,originLower);//Executa função auxiliar de escrita.
            saveFS(*fileSystem);//Salva o arquivo.
        }
        else {
            printf("Arquivo invalido para edicao.\n");
        }        
    }
}

//Move um arquivo para uma pasta designada
void move(char* srcPath, char* destPath, FS* fileSystem) {
    unsigned char originUpper, originLower, destLower;
    originUpper = originLower = destLower = VAZIO;
    
    if (destPath != NULL && destPath[0] != '\0') destLower = getDirIndex(destPath, *fileSystem); //getDirIndex da crash se receber '\0'
    getLastTwoIndex(srcPath, &originUpper, &originLower, *fileSystem);

    if(originUpper != VAZIO && originLower != VAZIO && destLower != VAZIO && (originLower != destLower)){
        if (isInDir(destLower, fileSystem->clusters[originLower].nome, fileSystem->clusters[originLower].tipo, *fileSystem) == VAZIO) {
            isInDir(originUpper, fileSystem->clusters[originLower].nome, fileSystem->clusters[originLower].tipo, *fileSystem);
            fseek(fileSystem->arquivo, (long int)(-1*sizeof(char)), SEEK_CUR);
            putc(VAZIO,fileSystem->arquivo);
            appendItem(*fileSystem, destLower, originLower);

            if (fileSystem->dirState.workingDirIndex == originLower)  {
                fileSystem->dirState.workingDirIndex = 0;
                strcpy(fileSystem->dirState.workingDir, "root");
            }
        }
        else printf("O diretorio de destino ja contem um arquivo com o mesmo nome e tipo.\n");
    }
    else printf("Caminhos Invalidos.\n");
}

//Renomeia um arquivo
void renameFile(char* path, char* name, FS* fileSystem) {//Função renameFile. Executa o comando RENAME. Recebe o caminho do arquivo, o nome novo e o fileSystem.
    unsigned char originUpper, originLower;
    int i;

    if (name == NULL) {
        printf("Termos invalidos\n");
        return;
    }
    i = strlen(name);  
    if (name[i-EXTENSION_SIZE] == '.'){//Caso o usuário insira um nome com fim .txt
        name[i-EXTENSION_SIZE] = '\0';
    }

    getLastTwoIndex(path, &originUpper, &originLower, *fileSystem);
    
    if (validateName(name) == 1) {
        if (originUpper == VAZIO){//Caso o diretório não exista, executa:
            printf("Esse diretorio nao existe\n");//Prompt de erro em caso de diretório incorreto.
        }else{//Se não, caso normal:
            if (originLower == VAZIO){            
                printf("Arquivo invalido.\n");//Prompt de erro em caso de arquivo inválido.           
            }else{//Se não, caso normal:
                if (isInDir(originUpper, name, fileSystem->clusters[originLower].tipo, *fileSystem)==VAZIO){
                    strcpy(fileSystem->clusters[originLower].nome,name);//Executa a troca de nome.
                    if (originLower == fileSystem->dirState.workingDirIndex) {
                        char upperPath[MAX_PATHNAME_SIZE];
                        char filler[MAX_FILENAME_SIZE];
                        separatePaths(path, upperPath, filler);
                        strcat(upperPath, "/");
                        strcat(upperPath, name);
                        strcpy(fileSystem->dirState.workingDir, upperPath);//Executa a troca de nome.
                    }
                    saveFS(*fileSystem);//Salva o arquivo.
                }else{
                    printf("Nome invalido para renomear. Detalhe: nome ja utilizado.\n");//Prompt de erro em caso de nome inválido.
                } 
            }
        }
    }else{
        printf("Nome invalido para renomear. Detalhe: nome com barras.\n");//Prompt de erro em caso de nome inválido.
    }
}

//Desfragmenta o disco, movendo todos os cluster para o início da área de dados
void defrag(FS* fileSystem) {
    unsigned char emptyIndex, usedIndex;
    int counter = 0;
    do {
        emptyIndex = findNextOpenCluster(*fileSystem);
        usedIndex = findLastUsedCluster(*fileSystem);
        if (usedIndex > emptyIndex) {
            moveCluster(fileSystem, usedIndex, emptyIndex);
            counter++;
        }
    } while (usedIndex > emptyIndex);
    printf("Desfragmentacao completa. Movidos %d arquivos\n", counter);
}
    
//Remove um arquivo. Se for um diretório com algum conteúdo, remove em cascata
void rf(char* path, FS* fileSystem) {
    unsigned char c,upper,lower;
    int i;
    char pathBranch[MAX_PATHNAME_SIZE];
    c = upper = lower = VAZIO;

    getLastTwoIndex(path, &upper, &lower, *fileSystem);
    //If executa se não for um diretório ou, caso contrário, testa se está vazio.
    if (!(strcmp("dir",fileSystem->clusters[lower].tipo) == 0) || dirIsEmpty(lower, *fileSystem)) {
        rm(path,fileSystem);
    }
    else { //Percorre cada arquivo dentro do diretório, chamando rf em cascata para cada um deles
        for (i = 0; c!=END_OF_FILE || (i>=fileSystem->meta.tam_cluster); i++) {
            setPointerToCluster(*fileSystem, lower);
            fseek(fileSystem->arquivo, (long int)(i*sizeof(char)), SEEK_CUR);     
            fread(&c, sizeof(unsigned char), 1, fileSystem->arquivo);
            if (c!=VAZIO && c!=END_OF_FILE) {
                sprintf(pathBranch,"%s/%s",path,fileSystem->clusters[c].nome);
                if (!(strcmp("dir",fileSystem->clusters[c].tipo) == 0)) {
                    sprintf(pathBranch,"%s.%s",pathBranch,fileSystem->clusters[c].tipo);
                }
                rf(pathBranch,fileSystem);
            }
        }
        rm(path,fileSystem);
    }
}

//Exibe o tamanho em kb ocupado pelo dir atual
void disk(FS fileSystem){
    // Chama a funcao getDirSize e imprime o valor na tela
    printf("Valor ocupado: %d Kb\n", TAM_CLUSTER/1000 * getDirSize(fileSystem,fileSystem.dirState.workingDirIndex));
}
