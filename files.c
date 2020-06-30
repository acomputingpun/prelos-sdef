#include <stdio.h>

#include "poses.h"
#include "nodes.h"
#include "files.h"

static int checkFileAttrs(FILE * stream, int oDepth, int autoDividePeriod);

NodeMemory readMemoryFileFrom(int oDepth, int autoDividePeriod, char* memoryPath) {
    char filename [120];
    sprintf(filename, "%snodes%d_%d.mem", memoryPath, oDepth, autoDividePeriod);

    FILE * stream = fopen(filename, "rb");
    if (stream == NULL) {
        printf("Unable to open file '%s' for reading!\n", filename);
        return NULL;
    }
    if (!checkFileAttrs(stream, oDepth, autoDividePeriod)) {
        fclose(stream);
        return NULL;
    }
    return nmUnflatten(stream);
}
int writeMemoryFileTo(NodeMemory nm, char* memoryPath) {
    char filename [120];
    sprintf(filename, "%snodes%d_%d.mem", memoryPath, nmDepth(nm), nmPeriod(nm));

    FILE * stream = fopen(filename, "wb");

    if (stream == NULL) {
        printf("Unable to open file '%s' for writing!\n", filename);
        return 0;
    }

    int versionID = VERSION_ID;
    if (!fwrite(&(versionID), sizeof(versionID), 1, stream)) { return 0; }
    if (!nmFlatten(nm, stream)) { return 0; }
    return 1;
}

NodeMemory readMemoryFile(int oDepth, int autoDividePeriod) {
    return readMemoryFileFrom(oDepth, autoDividePeriod, DEFAULT_MEMORY_PATH);
}
int writeMemoryFile(NodeMemory nm) {
    return writeMemoryFileTo(nm, DEFAULT_MEMORY_PATH);
}

static int checkFileAttrs(FILE * stream, int oDepth, int autoDividePeriod) {
    int check[3];
    if (!fread(check, sizeof(int), 3, stream)) {
        return 0;
    } else if (check[0] != VERSION_ID) {
        return 0;
    } else if (check[1] != oDepth) {
        return 0;
    } else if (check[2] != autoDividePeriod) {
        return 0;
    } else {
        return 1;
    }
}
