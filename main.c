#include <stdio.h>
#include <stdlib.h>
#include "fs.h"

int main() {
    FS fileSystem = initFS();
    const char spc[2] = " ";
    char input[210];
    char *cmd,*path;
    while (1) {
        printf("%s> ", fileSystem.dirState.workingDir);
        fgets(input, sizeof(input), stdin);
        input[strlen(input)-1] = '\0'; //Retira o '/n' do final

        cmd = strtok(input,spc);
        path = strtok(NULL,spc);

        if(!strcmp("CD",cmd)) cd(path, &fileSystem);
        else if(!strcmp("RM",cmd)) rm(path, &fileSystem);
        else if(!strcmp("MOVE",cmd)) move(path, strtok(NULL,spc),&fileSystem);
        else if(!strcmp("EXIT",cmd)) {
            fclose(fileSystem.arquivo);
            //TODO: definir exit(), a qual libera os espaços previamente alocados pela initFS()
            return 0;
        }
        else printf("Comando desconhecido.\n");
    }
}
