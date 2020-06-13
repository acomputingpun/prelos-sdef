#include <math.h>
#include <stdio.h>

#include "poses.h"
#include "tiles.h"
#include "rays.h"
#include "wedges.h"
#include "wdicts.h"
#include "nodes.h"
#include "wrapper.h"

NodeMemory losPrecompute(int oDepth, int autoDividePeriod) {
    printf ("Running w/ ADP %d, depth %d ", autoDividePeriod, oDepth);
    Octant oct = octCreate(oDepth);
    WedgeDict wdi = wdiCreate(oct);
    wdiBuild(oct, wdi, oDepth-1, autoDividePeriod);
    NodeMemory nm = nmCreate(wdi);
    return nm;
}