#include <stdio.h>
#include <stdlib.h>
#include "fs.h"

int main() {
    FS fileSystem = initFS();
    appendItem(fileSystem,1,30);

    return 0;
}
