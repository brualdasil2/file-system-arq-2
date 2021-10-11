#include <stdio.h>
#include <stdlib.h>
#include "fs.h"

int main() {
    FS fileSystem = initFS();
    mkdir("dir 1", fileSystem);
    mkdir("dir 2", fileSystem);
    mkfile("test", "txt", fileSystem);
    mkfile("test2", "txt", fileSystem);
    fclose(fileSystem.arquivo);
    return 0;
}
