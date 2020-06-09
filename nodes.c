#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "poses.h"
#include "rays.h"
#include "tiles.h"
#include "wedges.h"
#include "grids.h"
#include "nodes.h"

typedef struct node * Node;

struct nodeMemory {
    int nNodes;
    Node * matrix;
};
struct node {
    xyPos firstTile;
    int segmentLength;
    int ** childMap;
};

static void nCreate (NodeMemory nm, WedgeDict wdi, int wIndex);

NodeMemory nmCreate(WedgeDict wdi) {
    NodeMemory self = malloc(sizeof(struct nodeMemory));
    self->nNodes = 0;

    int nWedges = wdiNumWedges(wdi);

    for (int wIndex = 0; wIndex < nWedges; wIndex++) {
        printf(" -Visiting wedge %d!\n", wIndex);
        Wedge w = wdiLookupIndex(wdi, wIndex);
        if (w->mergedID == w->wedgeID) {
            w->finalNodeID = self->nNodes;
            self->nNodes++;
        }
    }

    printf("-Done initial runthrough\n");
    self->matrix = malloc(sizeof(Node *) * self->nNodes);

    for (int wIndex = 0; wIndex < nWedges; wIndex++) {
        printf(" -Revisiting wedge %d!\n", wIndex);
        Wedge w = wdiLookupIndex(wdi, wIndex);
        if (w->mergedID == w->wedgeID) {
            nCreate(self, wdi, wIndex);
        }
    }
    return self;
}
void nmDestroy(NodeMemory nm){
    return;
}
void nmPrint(NodeMemory nm){
    printf("Printing NodeMemory - it contains %d nodes!\n", nm->nNodes);
}


static void nCreate (NodeMemory nm, WedgeDict wdi, int wIndex) {
    Node n = malloc(sizeof(struct node));
    Wedge w = wdiLookupIndex(wdi, wIndex);

    printf("  -Just called nCreate on wedge index %d!\n", wIndex);

    n->segmentLength = w->segmentLength;
    n->firstTile = w->firstTile;
    n->childMap = malloc(sizeof(int) * getMaxBlockingBits(w));

    printf("  -Got firstTile (%d, %d), segmentLen %d, maxBB %x!\n", n->firstTile.x, n->firstTile.y, n->segmentLength, getMaxBlockingBits(w));

    for (unsigned int blockingBits = 0; blockingBits < getMaxBlockingBits(w); blockingBits++) {

        printf("   -Copying BlockingBits |%x|\n", blockingBits);

        int arrLen = w->childMap[blockingBits][0]+1;

        printf("   -ArrLen is %d\n", arrLen);
        printf("   -Arr is:");
        for (int k = 1; k < arrLen; k++) {
            printf("%d,", w->childMap[blockingBits][k]);
        }
        printf("\n");

        n->childMap[blockingBits] = malloc(sizeof(int) * (arrLen));
        for (int k = 1; k < arrLen; k++) {
            int childWedgeID = w->childMap[blockingBits][k];
            Wedge childWedge = wdiLookupIndex(wdi, wdiLookupIndex(wdi, childWedgeID)->mergedID);
            n->childMap[blockingBits][k] = childWedge->finalNodeID;
        }
    }

    nm->matrix[w->finalNodeID] = n;
}

void nmgRecast(NodeMemory nm, void * grid) {
    nmgRecastFrom(nm, grid, 0);
}

void nmgRecastFrom(NodeMemory nm, void * grid, int nodeID) {
    Node cur = nm->matrix[nodeID];
    int blockingBits = tDiagLookups(grid, cur->firstTile, cur->segmentLength);
    int * children = cur->childMap[blockingBits];
    for (int k = children[0]-1; k > 0; k++) {
        nmgRecastFrom(nm, grid, children[k]);
    }
}
