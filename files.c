#include <stdio.h>

#include "poses.h"
#include "tiles.h"
#include "rays.h"
#include "wedges.h"
#include "wdicts.h"
#include "nodes.h"
#include "files.h"

static int checkFileAttrs(FILE * stream, int oDepth, int autoDividePeriod);

NodeMemory readMemoryFile(int oDepth, int autoDividePeriod) {
    char filename [60];
    sprintf(filename, "./%d_%dnodes.mem", oDepth, autoDividePeriod);

    FILE * stream = fopen("rb", filename);
    if (stream == NULL) {
        return NULL;
    }
    if (!checkFileAttrs(stream, oDepth, autoDividePeriod)) {
        fclose(stream);
        return NULL;
    }
    return nmUnflatten(stream);
}
void writeMemoryFile(NodeMemory nm) {
    char filename [60];
    sprintf(filename, "./%d_%dnodes.mem", nmDepth(nm), nmPeriod(nm));

    FILE * stream = fopen("wb", filename);

    int versionID = VERSION_ID;
    fwrite(&(versionID), sizeof(versionID), 1, stream);
    nmFlatten(nm, stream);
}

static int checkFileAttrs(FILE * stream, int oDepth, int autoDividePeriod) {
    int check[3];
    fread(check, sizeof(int), 3, stream);
    if (check[0] != VERSION_ID) {
        return 0;
    } else if (check[1] != oDepth) {
        return 0;
    } else if (check[2] != autoDividePeriod) {
        return 0;
    } else {
        return 1;
    }
}
