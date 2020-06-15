#include <math.h>
#include <stdio.h>

#include "poses.h"
#include "tiles.h"
#include "rays.h"
#include "wedges.h"
#include "wdicts.h"
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
    printf ("Running w/ ADP %d, depth %d ", autoDividePeriod, oDepth);
    Octant oct = octCreate(oDepth, autoDividePeriod);
    WedgeDict wdi = wdiCreate(oct);
    wdiBuild(oct, wdi);
    NodeMemory nm = nmCreate(wdi);
    return nm;
}
