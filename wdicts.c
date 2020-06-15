#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "poses.h"
#include "rays.h"
#include "tiles.h"
#include "wedges.h"
#include "wdicts.h"

#define HASHTABLE_EXPAND_THRESHOLD 0.7

typedef struct wdRadix * WDRadix;

struct wedgeDict {
    int autoDividePeriod;
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

static void wdiIterativeBuild(Octant oct, WedgeDict wdi);

static void wdiRadixInsert(WDRadix self, Wedge wedge);
static void wdiRadixExpand(WDRadix self);


int wdiNextWedgeID(WedgeDict wdi){
    if (wdi->nWedges == wdi->indexArraySize) {
        wdi->indexArraySize *= 2;
//        printf("Reallocating for a total size of %d\n", wdi->indexArraySize);
        wdi->byIndex = realloc(wdi->byIndex, sizeof(Wedge) * wdi->indexArraySize);
    }
    return wdi->nWedges++;
}

WedgeDict wdiCreate(Octant oct) {
    WedgeDict self = malloc(sizeof(struct wedgeDict));
    self->autoDividePeriod = oct->autoDividePeriod;
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
        printf("DI %d - ", diagID);
        wdiRadixPrint(self->byDiagIDs[diagID]);
    }
    return;
}

int wdiDepth(WedgeDict self) {
    return self->nRadixes;
}
int wdiPeriod(WedgeDict self) {
    return self->autoDividePeriod;
}

static WDRadix wdiRadixCreate(Octant oct, int diagID){
    WDRadix self = malloc(sizeof(struct wdRadix));

    // A good estimation of radix size is that it increases by 10% per step?
    self->size = 32 << (diagID / 4);

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
    /*
    for (int cur = 0; cur < self->size; cur++) {
        if (self->byHashes[cur] != NULL) {
            printf("@");
        } else {
            printf(".");
        }
    }*/
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
//    printf("DI %d - ", diagID);
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

    if ( ((float)radix->d_elements) / ((float) radix->size) > HASHTABLE_EXPAND_THRESHOLD ) {
//        printf("Hashtable too full - radix %d has size %d with %d items filled (%d collisions)!  Expanding radix!\n", spec.diagID, radix->size, radix->d_elements, radix->d_collisions);
        wdiRadixExpand(radix);
    }

    int hash = hashSpec(spec);
    int iters = 0;
    for (int cur = hash % radix->size; iters++ <= radix-> size; cur = (cur+1) % radix->size) {
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
        }
    }
//    printf("Allocation error - radix %d has size %d!  Expanding radix!\n", spec.diagID, radix->size);
    wdiRadixExpand(radix);
    return wdiLookup(oct, self, spec);
}

static void wdiRadixInsert(WDRadix self, Wedge wedge) {
    int hash = hashSpec((*wedge));
    int iters = 0;
    for (int cur = hash % self->size; iters++ <= self->size; cur = (cur+1) % self->size) {
        if (self->byHashes[cur] == NULL) {
            self->byHashes[cur] = wedge;
            if (cur != hash % self->size) {
                self->d_collisions += 1;
            }
            return;
        }
    }
    printf("Unable to insert into expanded radix???  This should never occur!\n");
    exit(0);
}
static void wdiRadixExpand(WDRadix self) {
    self->d_collisions = 0;
    Wedge * oldByHashes = self->byHashes;
    self->size *= 2;
    self->byHashes = malloc( sizeof(Wedge) * self->size);
    for (int k = 0; k < self->size; k++) {
        self->byHashes[k] = NULL;
    }
    for (int k = 0; k < self->size / 2; k++) {
        if (oldByHashes[k] != NULL) {
            wdiRadixInsert(self, oldByHashes[k]);
        }
    }
    free (oldByHashes);
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

void wdiBuild(Octant oct, WedgeDict wdi) {
    wdiLookup(oct, wdi, wsInitial());
    wdiIterativeBuild(oct, wdi);
}

static void wdiIterativeBuild(Octant oct, WedgeDict wdi) {
    printf("Filling WDI\n");
    for (int wedgeID = 0; wedgeID < wdi->nWedges; wedgeID++) {
        wTraverse(oct, wdi, wdiLookupIndex(wdi, wedgeID));
    }
//    printf("DONE recursive traverse to depth %d\n", maxDepth);
}
