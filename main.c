#include <stdio.h>
#include <stdlib.h>
#include "fs.h"

int main() {
    FS fileSystem = initFS();
    
    renameFile("root/","tiago",fileSystem);
    edit("root/","teste",fileSystem);
    edit("tiago/","teste",fileSystem);

    return 0;
}
