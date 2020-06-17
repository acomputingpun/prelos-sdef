#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "poses.h"
#include "rays.h"
#include "grids.h"
#include "nodes.h"

static void nDestroy(Node n);
static void nPrint(Node n);
static void nFlatten(Node n, FILE * stream);
static Node nUnflatten(FILE * stream);

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

static void * fErrorHandle(FILE * stream) {
    return NULL;
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
    nmgRecastFrom(nm, grid, 0);
}

void nmgRecastFrom(NodeMemory nm, void * grid, int nodeID) {
    Node cur = nm->matrix[nodeID];
    int blockingBits = tDiagLookups(grid, cur->firstTile, cur->segmentLength);
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
    NodeMemory nm = malloc(sizeof(struct nodeMemory));
    if (! fread(&(nm->nNodes), sizeof(int), 1, stream)) {
        return fErrorHandle(stream);
    }

    nm->matrix = malloc(sizeof(Node) * nm->nNodes);
    for (int k = 0; k < nm->nNodes; k++) {
        if (! (nm->matrix[k] = nUnflatten(stream)) )  {
            return fErrorHandle(stream);
        }
    }
    return nm;
}

static Node nUnflatten(FILE * stream) {
    Node n = malloc(sizeof(struct node));
    if (! fread(&(n->firstTile), sizeof(xyPos), 1, stream) ) {
        return fErrorHandle(stream);
    };
    if (! fread(&(n->segmentLength), sizeof(int), 1, stream) ) {
        return fErrorHandle(stream);
    }

    int maxBlockingBits = getMaxBlockingBits(n);
    n->childMap = malloc(sizeof(int *) * maxBlockingBits);
    for (int blockingBits = 0; blockingBits < maxBlockingBits; blockingBits++) {
        int arrLen = 0;
        if (! fread(&arrLen, sizeof(int), 1, stream)) {
            return NULL;
        }

        int * children = malloc(sizeof(int) * (arrLen + 1));
        children[0] = arrLen;
        if (arrLen > 0) {
            if (!fread(children+1, sizeof(int), arrLen, stream)) {
                return NULL;
            }
        }
        n->childMap[blockingBits] = children;
    }
    return n;
}
