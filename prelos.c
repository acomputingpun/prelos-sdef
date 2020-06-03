#include <stdio.h>
#include <math.h>

#include "poses.h"
#include "tiles.h"
#include "rays.h"
#include "wedges.h"

void test() {
    int oDepth = 15;

    Octant o = octCreate(oDepth);
    octPrint(o);

    WedgeDict wdi = wdiCreate(o);
    wdiPrint(wdi);

    wdiLookup(o, wdi, wsInitial());

    wdiPrint(wdi);

    printf("Running recursive traverse!\n");

    wRecursiveTraverse(o, wdi, oDepth-1);

    wdiMergeEquivalent(wdi);

    wdiPrint(wdi);

    return;
}

int main (int argc, char** argv) {
    test();
    return 0;
}
