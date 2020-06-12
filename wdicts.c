#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "poses.h"
#include "rays.h"
#include "tiles.h"
#include "wedges.h"
#include "wdicts.h"

typedef struct wdRadix * WDRadix;

struct wedgeDict {
    int nRadixes;
    int nWedges;
    int indexArraySize;
    WDRadix * byDiagIDs;
    Wedge * byIndex;
};
struct wdRadix {
    int size;
    Wedge * byHashes;
    int d_collisions;
    int d_elements;
};

static WDRadix wdiRadixCreate(Octant oct, int diagID);
static void wdiRadixDestroy(WDRadix self);
static void wdiRadixPrint(WDRadix self);

static void wdiRadixMergeEquivalent(WedgeDict wdi, int diagID);

static void wdiIterativeBuild(Octant oct, WedgeDict wdi, int maxDepth, int autoDividePeriod);


int wdiNextWedgeID(WedgeDict wdi){
    if (wdi->nWedges == wdi->indexArraySize) {
        wdi->indexArraySize *= 2;
        wdi->byIndex = realloc(wdi->byIndex, sizeof(Wedge) * wdi->indexArraySize);
    }
    return wdi->nWedges++;
}

WedgeDict wdiCreate(Octant oct) {
    WedgeDict self = malloc(sizeof(struct wedgeDict));
    self->indexArraySize = 64;
    self->nWedges = 0;
    self->byIndex = malloc(sizeof(Wedge) * self->indexArraySize);
    self->nRadixes = oct->nDiags;
    self->byDiagIDs = malloc(sizeof(WDRadix) * self->nRadixes);
    for (int diagID = 0; diagID < self->nRadixes; diagID++) {
        self->byDiagIDs[diagID] = wdiRadixCreate(oct, diagID);
    }
    return self;
}
void wdiDestroy(WedgeDict self) {
    for (int diagID = 0; diagID < self->nRadixes; diagID++) {
        wdiRadixDestroy(self->byDiagIDs[diagID]);
    }
    free(self->byDiagIDs);
    free(self->byIndex);
    free(self);
}
void wdiPrint(WedgeDict self) {
    printf("WedgeDict with %d diag radixes\n", self->nRadixes);
    for (int diagID = 0; diagID < self->nRadixes; diagID++) {
        wdiRadixPrint(self->byDiagIDs[diagID]);
    }
    return;
}

static WDRadix wdiRadixCreate(Octant oct, int diagID){
    WDRadix self = malloc(sizeof(struct wdRadix));
    self->size = ((1 << oct->diags[diagID].size) * (diagID+1)) >> 1;
    self->byHashes = malloc(sizeof(Wedge) * self->size);
    for (int k = 0; k < self->size; k++) {
        self->byHashes[k] = NULL;
    }

    self->d_collisions = 0;
    self->d_elements = 0;

    return self;
}
static void wdiRadixDestroy(WDRadix self) {
    for (int cur = 0; cur < self->size; cur++) {
        if (self->byHashes[cur] != NULL) {
            wDestroy(self->byHashes[cur]);
        }
    }
    free(self->byHashes);
    free(self);
}
static void wdiRadixPrint(WDRadix self) {
    printf("Radix of size %d (%d elem, %d coll):", self->size, self->d_elements, self->d_collisions);
    for (int cur = 0; cur < self->size; cur++) {
        if (self->byHashes[cur] != NULL) {
            printf("@");
        } else {
            printf(".");
        }
    }
    printf("\n");
    /*
    for (int cur = 0; cur < self->size; cur++) {
        if (self->byHashes[cur] != NULL) {
            printf("  ");
            wPrint(self->byHashes[cur]);
            printf("\n");
        }
    }*/
}

///

void wdiMergeEquivalent(WedgeDict wdi) {
    for (int diagID = wdi->nRadixes-1; diagID >= 0; diagID--) {
        wdiRadixMergeEquivalent(wdi, diagID);
    }
}

int wdiNumWedges(WedgeDict wdi) {
    return wdi->nWedges;
}

static void wdiRadixMergeEquivalent(WedgeDict wdi, int diagID) {
//    printf("Running through and merging from radix of diag %d\n", diagID);

    WDRadix radix = wdi->byDiagIDs[diagID];
//    wdiRadixPrint(radix);

    for (int curIndex = 0; curIndex < radix->size; curIndex++) {
        Wedge curWedge = radix->byHashes[curIndex];
        if (curWedge != NULL && curWedge->mergedID == curWedge->wedgeID) {
            for (int otherIndex = curIndex+1; otherIndex < radix->size; otherIndex++) {
                Wedge otherWedge = radix->byHashes[otherIndex];
                if (otherWedge != NULL) {
                    if (wEquivalent(wdi, curWedge, otherWedge)) {
//                        printf("    ...they are equivalent - merging!\n");
                        otherWedge->mergedID = curWedge->mergedID;
                    }
                }
            }
        }
    }
}

///


Wedge wdiLookup(Octant oct, WedgeDict self, wedgeSpec spec) {
    WDRadix radix = self->byDiagIDs[spec.diagID];

    int hash = hashSpec(spec);
    for (int cur = hash % radix->size; ; cur = (cur+1) % radix->size) {
        if (radix->byHashes[cur] == NULL) {
            Wedge new = wCreate(oct, self, spec);
            radix->byHashes[cur] = new;
            self->byIndex[new->wedgeID] = new;
            radix->d_elements += 1;
            if (cur != hash % radix->size) {
                radix->d_collisions += 1;
            }
            return radix->byHashes[cur];
        } else if (wEqualsSpec(radix->byHashes[cur], spec)) {
            return radix->byHashes[cur];
        } else if (cur > radix->size) {
            return NULL;
        }
    }
    return NULL;
}

Wedge wdiLookupIndex(WedgeDict self, int wedgeID) {
//    printf("looking up index %d\n", wedgeID);
//    printf("sbi %p\n", self->byIndex);
    return self->byIndex[wedgeID];
}

static inline ray tileToCWRay(xyPos tile) {
    ray cwEdge;
    cwEdge.x = (tile.x * 2) + 1;
    cwEdge.y = (tile.y * 2) - 1;
    return cwEdge;
}
static inline ray tileToCCWRay(xyPos tile) {
    ray ccwEdge;
    ccwEdge.x = (tile.x * 2) - 1;
    ccwEdge.y = (tile.y * 2) + 1;
    return ccwEdge;
}

void wdiBuild(Octant oct, WedgeDict wdi, int maxDepth, int autoDividePeriod) {
    wdiLookup(oct, wdi, wsInitial());
    wdiIterativeBuild(oct, wdi, maxDepth, autoDividePeriod);
}

static void wdiIterativeBuild(Octant oct, WedgeDict wdi, int maxDepth, int autoDividePeriod) {
    for (int wedgeID = 0; wedgeID < wdi->nWedges; wedgeID++) {
        wTraverse(oct, wdi, wdiLookupIndex(wdi, wedgeID), maxDepth, autoDividePeriod);
    }
//    printf("DONE recursive traverse to depth %d\n", maxDepth);
}
