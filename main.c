#include <stdio.h>
#include <stdlib.h>
#include "fs.h"

int main() {
    FS fileSystem = initFS();
    const char spc[2] = " ";
    char input[210];
    char *cmd,*path;
    fclose(fileSystem.arquivo);
    while (1) {
        fileSystem.arquivo = fopen(NOME_ARQUIVO, "rb+"); //TIRAR DEPOIS!!!! TA SO PRA TESTE
        printf("%s> ", fileSystem.dirState.workingDir);
        fgets(input, sizeof(input), stdin);
        input[strlen(input)-1] = '\0'; //Retira o '/n' do final

        cmd = strtok(input,spc);
        path = strtok(NULL,spc);

        if(!strcmp("CD",cmd)) {
            cd(path, &fileSystem);
        }
        else if(!strcmp("DIR",cmd)) {
            dir(fileSystem);
        }
        else if(!strcmp("RM",cmd)) {
            rm(path, &fileSystem);
        }
        else if(!strcmp("MKDIR",cmd)) {
            mkdir(path, &fileSystem);
        }
        else if(!strcmp("MKFILE",cmd)) {
            mkfile(path, &fileSystem);
        }
        else if(!strcmp("EDIT",cmd)) {
            edit(path, strtok(NULL,spc), &fileSystem);
        }
        else if(!strcmp("MOVE",cmd)) { 
            move(path, strtok(NULL,spc), &fileSystem);
        }
        else if(!strcmp("RENAME",cmd)) { 
            renameFile(path, strtok(NULL,spc), &fileSystem);
        }
        else if(!strcmp("EXIT",cmd)) {
            fclose(fileSystem.arquivo);
            free(fileSystem.indice);
            free(fileSystem.clusters);
            return 0;
        }
        else printf("Comando desconhecido.\n");

        fclose(fileSystem.arquivo);
    }
}
