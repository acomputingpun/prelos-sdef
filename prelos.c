#include <stdio.h>
#include <math.h>

#include "poses.h"
#include "tiles.h"
#include "rays.h"
#include "wedges.h"

void test() {
    Octant o = octCreate(5);
    octPrint(o);

    WedgeDict wdi = wdiCreate(o);
    wdiPrint(wdi);

    wdiLookup(o, wdi, wsInitial());

    wdiPrint(wdi);

    printf("Running recursive traverse!\n");

    wRecursiveTraverse(o, wdi, 3);

    return;
}

int main (int argc, char** argv) {
    test();
    return 0;
}
