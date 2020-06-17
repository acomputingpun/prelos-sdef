#include <math.h>
#include <stdio.h>

#include "poses.h"
#include "rays.h"
#include "nodes.h"
#include "wrapper.h"
#include "files.h"

NodeMemory losLookup(int oDepth, int autoDividePeriod) {
    NodeMemory nm = readMemoryFile(oDepth, autoDividePeriod);
    if (nm == NULL) {
        nm = losPrecompute(oDepth, autoDividePeriod);
        writeMemoryFile(nm);
    }
    return nm;
}

NodeMemory losPrecompute(int oDepth, int autoDividePeriod) {
    printf("Unable to precompute LOS; read-only library lacks generation code!");
    return NULL;
}
