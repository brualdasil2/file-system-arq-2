#include <stdio.h>
#include <stdlib.h>
#include "fs.h"

int main() {
    //printf("Hello World!");

    FS fileSystem = initFS();

    printf("%d\n", (unsigned short) fileSystem.meta.tam_indice);
    printf("%d\n", (unsigned short) fileSystem.meta.tam_cluster);
    printf("%d\n", (unsigned short) fileSystem.meta.ini_indice);
    printf("%d\n", (unsigned short) fileSystem.meta.ini_root);
    printf("%x\n", (unsigned char) fileSystem.indice[0]);
    printf("%x\n", (unsigned char) fileSystem.indice[54]);
    printf("%s\n", fileSystem.clusters[0].nome);
    printf("%s\n", fileSystem.clusters[0].tipo);

    return 0;
}
