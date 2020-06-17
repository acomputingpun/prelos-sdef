#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "poses.h"
#include "rays.h"
#include "tiles.h"
#include "wedges.h"
#include "wdicts.h"
#include "grids.h"
#include "nodes.h"

static size_t nCreate(NodeMemory nm, WedgeDict wdi, int wIndex);

NodeMemory nmCreate(WedgeDict wdi) {
    NodeMemory self = malloc(sizeof(struct nodeMemory));
    size_t datasize = 0;
    self->nNodes = 0;

    self->oDepth = wdiDepth(wdi);
    self->autoDividePeriod = wdiPeriod(wdi);

    int nWedges = wdiNumWedges(wdi);

    for (int wIndex = 0; wIndex < nWedges; wIndex++) {
        Wedge w = wdiLookupIndex(wdi, wIndex);
        if (w->mergedID == w->wedgeID) {
            w->finalNodeID = self->nNodes;
            self->nNodes++;
        }
    }

    self->matrix = malloc(sizeof(Node *) * self->nNodes);

    for (int wIndex = 0; wIndex < nWedges; wIndex++) {
        Wedge w = wdiLookupIndex(wdi, wIndex);
        if (w->mergedID == w->wedgeID) {
            datasize += nCreate(self, wdi, wIndex);
        }
    }

    wdiDestroy(wdi);

    printf("Final datasize: %d nodes, %ld bytes (%ld kb)\n", self->nNodes, datasize, datasize >> 10);

    return self;
}

static size_t nCreate (NodeMemory nm, WedgeDict wdi, int wIndex) {
    size_t datasize = 0;
    Wedge w = wdiLookupIndex(wdi, wIndex);
    Node n = malloc(sizeof(struct node));
    datasize += sizeof(struct node);

    n->segmentLength = w->segmentLength;
    n->firstTile = w->firstTile;
    n->childMap = malloc(sizeof(int *) * getMaxBlockingBits(w));
    datasize += sizeof(int *) * getMaxBlockingBits(w);

    for (unsigned int blockingBits = 0; blockingBits < getMaxBlockingBits(w); blockingBits++) {

        int arrLen = w->childMap[blockingBits][0]+1;

        n->childMap[blockingBits] = malloc(sizeof(int) * (arrLen));
        datasize += sizeof(int) * (arrLen);
        n->childMap[blockingBits][0] = arrLen-1;
        for (int k = 1; k < arrLen; k++) {
            int childWedgeID = w->childMap[blockingBits][k];
            Wedge childWedge = wdiLookupIndex(wdi, wdiLookupIndex(wdi, childWedgeID)->mergedID);
            n->childMap[blockingBits][k] = childWedge->finalNodeID;
        }
    }
    nm->matrix[w->finalNodeID] = n;
    return datasize;
}
