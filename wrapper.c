#include <math.h>
#include <stdio.h>

#include "poses.h"
#include "tiles.h"
#include "rays.h"
#include "wedges.h"
#include "wdicts.h"
#include "nodes.h"
#include "create_nodes.h"
#include "wrapper.h"
#include "files.h"

NodeMemory losLookupFrom(int oDepth, int autoDividePeriod, char* memoryPath) {
    NodeMemory nm = readMemoryFileFrom(oDepth, autoDividePeriod, memoryPath);
    if (nm == NULL) {
        nm = losPrecompute(oDepth, autoDividePeriod);
        if (!writeMemoryFileTo(nm, memoryPath)) {
            nmDestroy(nm);
            return NULL;
        }
    }
    return nm;
}

NodeMemory losLookup(int oDepth, int autoDividePeriod) {
    return losLookupFrom(oDepth, autoDividePeriod, DEFAULT_MEMORY_PATH);
}

NodeMemory losPrecompute(int oDepth, int autoDividePeriod) {
    Octant oct = octCreate(oDepth, autoDividePeriod);
    WedgeDict wdi = wdiCreate(oct);
    wdiBuild(oct, wdi);
    NodeMemory nm = nmCreate(wdi);
    return nm;
}
