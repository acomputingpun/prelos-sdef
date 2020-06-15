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

typedef struct node * Node;

struct nodeMemory {
    int oDepth;
    int autoDividePeriod;
    int nNodes;
    Node * matrix;
};
struct node {
    xyPos firstTile;
    int segmentLength;
    int ** childMap;
};

static size_t nCreate(NodeMemory nm, WedgeDict wdi, int wIndex);
static void nDestroy(Node n);
static void nPrint(Node n);
static void nFlatten(Node n, FILE * stream);
static Node nUnflatten(FILE * stream);

NodeMemory nmCreate(WedgeDict wdi) {
    NodeMemory self = malloc(sizeof(struct nodeMemory));
    size_t datasize = 0;
    self->nNodes = 0;

    self->oDepth = wdiDepth(wdi);
    self->autoDividePeriod = wdiPeriod(wdi);

    int nWedges = wdiNumWedges(wdi);

    for (int wIndex = 0; wIndex < nWedges; wIndex++) {
//        printf(" -Visiting wedge %d!\n", wIndex);
        Wedge w = wdiLookupIndex(wdi, wIndex);
        if (w->mergedID == w->wedgeID) {
            w->finalNodeID = self->nNodes;
            self->nNodes++;
        }
    }

//    printf("-Done initial runthrough\n");
    self->matrix = malloc(sizeof(Node *) * self->nNodes);

    for (int wIndex = 0; wIndex < nWedges; wIndex++) {
//        printf(" -Revisiting wedge %d!\n", wIndex);
        Wedge w = wdiLookupIndex(wdi, wIndex);
        if (w->mergedID == w->wedgeID) {
            datasize += nCreate(self, wdi, wIndex);
        }
    }

    wdiDestroy(wdi);

    printf("Final datasize: %d nodes, %ld bytes (%ld kb)\n", self->nNodes, datasize, datasize >> 10);

    return self;
}
void nmDestroy(NodeMemory nm){
    for (int nodeID = 0; nodeID < nm->nNodes; nodeID++) {
        nDestroy(nm->matrix[nodeID]);
    }
    free(nm->matrix);
    free(nm);
    return;
}
void nmPrint(NodeMemory nm){
    printf("Printing NodeMemory - it contains %d nodes!\n", nm->nNodes);
    for (int nodeID = 0; nodeID < nm->nNodes; nodeID++) {
        printf("%d:", nodeID);
        nPrint(nm->matrix[nodeID]);
    }
}

int nmDepth(NodeMemory nm) {
    return nm->oDepth;
}
int nmPeriod(NodeMemory nm) {
    return nm->autoDividePeriod;
}

static size_t nCreate (NodeMemory nm, WedgeDict wdi, int wIndex) {
    size_t datasize = 0;
    Wedge w = wdiLookupIndex(wdi, wIndex);
    Node n = malloc(sizeof(struct node));
    datasize += sizeof(struct node);


//    printf("  -Just called nCreate on wedge index %d!\n", wIndex);

    n->segmentLength = w->segmentLength;
    n->firstTile = w->firstTile;
    n->childMap = malloc(sizeof(int *) * getMaxBlockingBits(w));
    datasize += sizeof(int *) * getMaxBlockingBits(w);

//    printf("  -Got firstTile (%d, %d), segmentLen %d, maxBB %x!\n", n->firstTile.x, n->firstTile.y, n->segmentLength, getMaxBlockingBits(w));

    for (unsigned int blockingBits = 0; blockingBits < getMaxBlockingBits(w); blockingBits++) {

        int arrLen = w->childMap[blockingBits][0]+1;

//        printf("   -Copying BlockingBits |%x| (arrLen %d)\n", blockingBits, arrLen-1);

/*        printf("   -ArrLen is %d\n", arrLen);
        printf("   -Arr is:");
        for (int k = 1; k < arrLen; k++) {
            printf("%d,", w->childMap[blockingBits][k]);
        }
        printf("\n");*/

        n->childMap[blockingBits] = malloc(sizeof(int) * (arrLen));
        datasize += sizeof(int) * (arrLen);
        n->childMap[blockingBits][0] = arrLen-1;
        for (int k = 1; k < arrLen; k++) {
//            printf("    -Copying k %d\n", k);
            int childWedgeID = w->childMap[blockingBits][k];
            Wedge childWedge = wdiLookupIndex(wdi, wdiLookupIndex(wdi, childWedgeID)->mergedID);
            n->childMap[blockingBits][k] = childWedge->finalNodeID;
        }
//        printf("   -done w/ bb %x\n", blockingBits);
    }
//    printf("  -Done copying!\n");
    nm->matrix[w->finalNodeID] = n;
    return datasize;
}
static void nDestroy(Node n) {
    for(unsigned int blockingBits = 0; blockingBits < getMaxBlockingBits(n); blockingBits++){
        free(n->childMap[blockingBits]);
    }
    free(n->childMap);
    free(n);
}
static void nPrint(Node n) {
    printf("Node (%d,%d) %d -> (%d,%d)\n", n->firstTile.x, n->firstTile.y, n->segmentLength, 1+n->firstTile.x-n->segmentLength, n->firstTile.y+n->segmentLength-1);
    for(unsigned int blockingBits = 0; blockingBits < getMaxBlockingBits(n); blockingBits++){
        printf("    %x | (", blockingBits);
        for (int k = 1; k <= n->childMap[blockingBits][0]; k++) {
            printf("%d, ", n->childMap[blockingBits][k]);
        }
        printf(")\n");
    }
}


void nmgRecast(NodeMemory nm, void * grid) {
//    printf("Called nmgRecast w/ NM %p, grid %p\n", nm, grid);
    nmgRecastFrom(nm, grid, 0);
}

void nmgRecastFrom(NodeMemory nm, void * grid, int nodeID) {
//    printf(" Recast: at node %d\n", nodeID);
    Node cur = nm->matrix[nodeID];
    int blockingBits = tDiagLookups(grid, cur->firstTile, cur->segmentLength);
//    printf(" Recast: Got blockingBits %x\n", blockingBits);
    int * children = cur->childMap[blockingBits];
    for (int k = children[0]; k > 0; k--) {
        nmgRecastFrom(nm, grid, children[k]);
    }
}

void nmFlatten(NodeMemory nm, FILE * stream) {
    fwrite(&(nm->oDepth), sizeof(int), 1, stream);
    fwrite(&(nm->autoDividePeriod), sizeof(int), 1, stream);

    fwrite(&(nm->nNodes), sizeof(int), 1, stream);

    for (int k = 0; k < nm->nNodes; k++) {
        nFlatten(nm->matrix[k], stream);
    }

    return;
}

static void nFlatten(Node n, FILE * stream) {
    fwrite(&(n->firstTile), sizeof(xyPos), 1, stream);
    fwrite(&(n->segmentLength), sizeof(int), 1, stream);
    int maxBlockingBits = getMaxBlockingBits(n);
    for (int blockingBits = 0; blockingBits < maxBlockingBits; blockingBits++) {
        int * children = n->childMap[blockingBits];
        fwrite(children, sizeof(int), children[0]+1, stream);
    }
}

NodeMemory nmUnflatten(FILE * stream) {
//    printf("Unflattening nodeMemory!\n");

    NodeMemory nm = malloc(sizeof(struct nodeMemory));
    fread(&(nm->nNodes), sizeof(int), 1, stream);

//    printf("Read nNodes: %d\n", nm->nNodes);

    nm->matrix = malloc(sizeof(Node) * nm->nNodes);
    for (int k = 0; k < nm->nNodes; k++) {
        nm->matrix[k] = nUnflatten(stream);
    }
    return nm;
}

static Node nUnflatten(FILE * stream) {
//    printf(" - Unflattening node!\n");

    Node n = malloc(sizeof(struct node));
    fread(&(n->firstTile), sizeof(xyPos), 1, stream);
//    printf("  - Read firstTile: %d, %d\n", n->firstTile.x, n->firstTile.y);
    fread(&(n->segmentLength), sizeof(int), 1, stream);
//    printf("  - Read segmentLength: %d\n", n->segmentLength);

    int maxBlockingBits = getMaxBlockingBits(n);
    n->childMap = malloc(sizeof(int *) * maxBlockingBits);
    for (int blockingBits = 0; blockingBits < maxBlockingBits; blockingBits++) {
//        printf("  - Reading for BB %x\n", blockingBits);

        int arrLen = 0;
        fread(&arrLen, sizeof(int), 1, stream);
//        printf("    - Got arrLen %d\n", arrLen);

        int * children = malloc(sizeof(int) * (arrLen + 1));
        children[0] = arrLen;
        if (arrLen > 0) {
            /*
            for (int k = 0; k <= arrLen; k++) {
                printf("     - Children[%d] is: %d\n", k, children[k]);
            }
            printf("Children[1] is %p\n", &(children[1]));
            printf("Children +1 is %p\n", children+1);*/
            fread(children+1, sizeof(int), arrLen, stream);
        }
        n->childMap[blockingBits] = children;
    }
    return n;
}
