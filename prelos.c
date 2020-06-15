#include <stdio.h>
#include <math.h>

#include "poses.h"
#include "tiles.h"
#include "rays.h"
#include "wedges.h"
#include "wdicts.h"
#include "nodes.h"

#include "grids.h"
#include "localgrids.h"

void test() {
    int oDepth = 15;

    Octant o = octCreate(oDepth, 8);
    octPrint(o);

    WedgeDict wdi = wdiCreate(o);
    wdiPrint(wdi);

    printf("Running recursive traverse!\n");

    wdiBuild(o, wdi);

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

    Octant oct = octCreate(8, 8);
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
    int oDepth = 3;

    void * grid = setupGrid();

    Octant oct = octCreate(oDepth, 8);
    octPrint(oct);

    printf("Done creating octant, now filling grid!\n");

    for (int nDiag = 0; nDiag < oct->nDiags; nDiag++) {
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

    wdiBuild(oct, wdi);

    wdiPrint(wdi);

    printf("Creating node-memory from wdi!\n");

    NodeMemory nm = nmCreate(wdi);

    printf("Done creating node-memory, now printing:\n");

    nmPrint(nm);

    nmgRecast(nm, grid);
}

void createNodes(int oDepth, int autoDividePeriod, int ME) {
    printf("Creating node-memory with ADP %d / depth %d / mergeq %d:\n", autoDividePeriod, oDepth, ME);
    Octant oct = octCreate(oDepth, autoDividePeriod);
    WedgeDict wdi = wdiCreate(oct);

    wdiBuild(oct, wdi);

    if (ME) {
        wdiMergeEquivalent(wdi);
    }

    NodeMemory nm = nmCreate(wdi);
    nmDestroy(nm);
    octDestroy(oct);
}

void testSizes() {
    for (int adp = 6; adp <= 12; adp++) {
        printf ("-- ADP %d\n", adp);
        for (int k = 50; k <= 50; k+= 5) {
//            createNodes(k, adp, 0);
            createNodes(k, adp, 1);
//            system("free -hm");
        }
    }
}

void testMemory() {
    int oDepth = 7;

    printf("Testing octant!\n");

    Octant oct = octCreate(oDepth, 7);
    octPrint(oct);
    octDestroy(oct);

    printf("Testing grid!\n");

    void * grid = setupGrid();
    gridPrint(grid);
    gridDestroy(grid);

    printf("Testing empty WDI!\n");

    oct = octCreate(oDepth, 7);
    WedgeDict wdi = wdiCreate(oct);
    wdiPrint(wdi);
    wdiDestroy(wdi);
    octDestroy(oct);


    printf("Testing full WDI!\n");

    oct = octCreate(oDepth, 7);
    wdi = wdiCreate(oct);
    wdiPrint(wdi);
    wdiBuild(oct, wdi);
    wdiDestroy(wdi);
    octDestroy(oct);

    printf("Testing WDI->NodeMemory\n");

    oct = octCreate(oDepth, 7);
    wdi = wdiCreate(oct);
    wdiPrint(wdi);
    wdiBuild(oct, wdi);
    NodeMemory nm = nmCreate(wdi);
    nmDestroy(nm);
    octDestroy(oct);
}

int main (int argc, char** argv) {
//    testGrid();
//    testRecast();
//    testMemory();
    testSizes();
    return 0;
}
