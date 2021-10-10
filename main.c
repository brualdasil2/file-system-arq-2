#include <stdio.h>
#include <stdlib.h>
#include "fs.h"

int main() {
    FS fileSystem = initFS();
    unsigned char a = findNextOpenCluster(fileSystem);
    printf("0x%.8X", a);
    return 0;
}
