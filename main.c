#include <stdio.h>
#include <stdlib.h>
#include "fs.h"

int main() {
    FS fileSystem = initFS();
    make("dir1","dir", fileSystem);
    make("/root/dir1/dir2","dir", fileSystem);
    make("/root/dir1/dir2/test","txt", fileSystem);
    make("testRoot","txt", fileSystem);
    fclose(fileSystem.arquivo);
    return 0;
}
