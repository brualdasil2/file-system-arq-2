#include <stdio.h>
#include <stdlib.h>
#include "fs.h"

int main() {
    FS fileSystem = initFS();
    const char spc[2] = " ";
    const char dQuotes[2] = "\"";
    int i, quoteFinder;
    char input[210];
    char inputClone[210];
    char *cmd,*path,*editContent;
    fclose(fileSystem.arquivo);
    while (1) {
        fileSystem.arquivo = fopen(NOME_ARQUIVO, "rb+"); //TIRAR DEPOIS!!!! TA SO PRA TESTE
        printf("%s> ", fileSystem.dirState.workingDir);
        fgets(input, sizeof(input), stdin);
        input[strlen(input)-1] = '\0'; //Retira o '/n' do final
        strcpy(inputClone,input);

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
            quoteFinder = 0;
            for(i=0;i < strlen(inputClone)+1;i++){                
                if(inputClone[i]=='\"'){
                    quoteFinder = 1;
                }
            }
            if(quoteFinder){
                editContent = strtok(inputClone,dQuotes);
                editContent = strtok(NULL,dQuotes);         
            }else{
                editContent = strchr(inputClone,' ')+1;  
                editContent = strchr(editContent,' ')+1;               
            }     
            edit(path, editContent, &fileSystem);         
        }
        else if(!strcmp("MOVE",cmd)) { 
            move(path, strtok(NULL,spc), &fileSystem);
        }
        else if(!strcmp("RENAME",cmd)) {
            renameFile(path, strtok(NULL,spc), &fileSystem);
        }
        else if(!strcmp("DISK",cmd)){
            disk(fileSystem);
        }
        else if(!strcmp("EXIT",cmd)) {
            return closeFS(fileSystem.arquivo, fileSystem.indice, fileSystem.clusters);          
        }
        else printf("Comando desconhecido.\n");

        fclose(fileSystem.arquivo);   
    }
}
