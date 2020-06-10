#include <stdio.h>

#include "nodes.h"
#include "files.h"

NodeMemory readMemoryFile() {
    FILE * stream = fopen("rb", "./nodes.mem");
    return nmUnflatten(stream);
}
void writeMemory(NodeMemory nm) {
    FILE * stream = fopen("wb", "./nodes.mem");
    nmFlatten(nm, stream);
}
