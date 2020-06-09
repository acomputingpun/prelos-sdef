#include <stdio.h>
#include <math.h>

#include "poses.h"
#include "tiles.h"
#include "rays.h"
#include "wedges.h"
#include "nodes.h"

#include "grids.h"
#include "localgrids.h"

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

void * setupGrid() {
    xyPos size;
    size.x = 12;
    size.y = 12;
    void * grid = gridCreate( size );
    return grid;
}

void testGrid() {
    void * grid = setupGrid();

    Octant oct = octCreate(8);
    octPrint(oct);

    for (int nDiag = 0; nDiag <= oct->nDiags; nDiag++) {
        diagonal diag = oct->diags[nDiag];
        tDiagSet(grid, oct->tilePoses[diag.firstTile], diag.size, 0);
    }

    xyPos x = {2, 0};
    tSet(grid, x, '#');

    gridPrint(grid);
}

void testRecast() {
    int oDepth = 7;

    void * grid = setupGrid();

    Octant oct = octCreate(oDepth);
    octPrint(oct);

    for (int nDiag = 0; nDiag <= oct->nDiags; nDiag++) {
        diagonal diag = oct->diags[nDiag];
        tDiagSet(grid, oct->tilePoses[diag.firstTile], diag.size, '.');
    }

    xyPos x = {2, 0};
    tSet(grid, x, '#');

    gridPrint(grid);

    printf("Creating wdi!\n");

    WedgeDict wdi = wdiCreate(oct);
    wdiPrint(wdi);

    printf("Running recursive traverse on wdi!\n");

    wdiLookup(oct, wdi, wsInitial());

    wRecursiveTraverse(oct, wdi, oDepth-1);

    wdiPrint(wdi);

    printf("Creating node-memory from wdi!\n");

    NodeMemory nm = nmCreate(wdi);

    nmPrint(nm);
}

int main (int argc, char** argv) {
//    testGrid();
    testRecast();
//    test();
    return 0;
}
