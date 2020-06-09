#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "poses.h"
#include "rays.h"
#include "tiles.h"
#include "wedges.h"

#define wedgeCWTile(wedge) (wedge->cwEdge)
#define wedgeCCWTile(wedge) (wedge->ccwEdge)

#define hashSpec(spec) ((37 * hashRay(spec.cwEdge)) ^ (101*hashRay(spec.ccwEdge)))

#define getMaxBlockingBits(w) (1 << w->segmentLength)

typedef struct wdRadix * WDRadix;

struct wedgeSpecList {
    wedgeSpec data;
    WedgeSpecList next;
};
static WedgeSpecList wslPush(WedgeSpecList self, wedgeSpec data);

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

static Wedge wCreate(Octant oct, WedgeDict wdi, wedgeSpec spec);
static void wDestroy(Wedge self);
static int wdiNextWedgeID(WedgeDict wdi);

static WDRadix wdiRadixCreate(Octant oct, int diagID);
static void wdiRadixDestroy(WDRadix self);
static void wdiRadixPrint(WDRadix self);

static WedgeSpecList wBitsToChildSpecs(Wedge wedge, unsigned int blockingBits);
static int wslLength(WedgeSpecList self);

void wsPrint(wedgeSpec ws);
void wslPrint(WedgeSpecList self);

static void wTraverse(Octant oct, WedgeDict wdi, Wedge w, int maxDepth);
static void wInnerTraverse(Octant oct, WedgeDict wdi, Wedge w, unsigned int blockingBits);

static int wEqualsSpec(Wedge self, wedgeSpec spec) {
    return rayEquals(self->cwEdge, spec.cwEdge) && rayEquals(self->ccwEdge, spec.ccwEdge) && (self->diagID == spec.diagID);
}

static Wedge wCreate(Octant oct, WedgeDict wdi, wedgeSpec spec) {
    Wedge self = malloc(sizeof(struct wedge));
    self->cwEdge = spec.cwEdge;
    self->ccwEdge = spec.ccwEdge;
    self->diagID = spec.diagID;

    self->firstTile = cwDiagIntersectTile( self->cwEdge, self->diagID);
    xyPos lastTile = ccwDiagIntersectTile( self->ccwEdge, self->diagID);
    self->segmentLength = (self->firstTile.x - lastTile.x) + 1;

    self->wedgeID = wdiNextWedgeID(wdi);
    wdi->byIndex[self->wedgeID] = self;
    self->mergedID = self->wedgeID;

    self->childMap = malloc(sizeof(int *) * getMaxBlockingBits(self) );
    return self;
}
static void wDestroy(Wedge self) {
    free(self);
}

static int wdiNextWedgeID(WedgeDict wdi){
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

static int wEquivalent(WedgeDict wdi, Wedge w1, Wedge w2);
static int cmEquivalent(WedgeDict wdi, int * cm1, int * cm2);
static void wdiRadixMergeEquivalent(WedgeDict wdi, int diagID);

void wdiMergeEquivalent(WedgeDict wdi) {
    for (int diagID = wdi->nRadixes-1; diagID >= 0; diagID--) {
        wdiRadixMergeEquivalent(wdi, diagID);
    }
}

static void wdiRadixMergeEquivalent(WedgeDict wdi, int diagID) {
    printf("Running through and merging from radix of diag %d\n", diagID);

    WDRadix radix = wdi->byDiagIDs[diagID];
    wdiRadixPrint(radix);

    for (int curIndex = 0; curIndex < radix->size; curIndex++) {
        Wedge curWedge = radix->byHashes[curIndex];
        if (curWedge != NULL && curWedge->mergedID == curWedge->wedgeID) {
            for (int otherIndex = curIndex+1; otherIndex < radix->size; otherIndex++) {
                Wedge otherWedge = radix->byHashes[otherIndex];
                if (otherWedge != NULL) {
                    if (wEquivalent(wdi, curWedge, otherWedge)) {
                        printf("    ...they are equivalent - merging!\n");
                        otherWedge->mergedID = curWedge->mergedID;
                    }
                }
            }
        }
    }
}

static int wEquivalent(WedgeDict wdi, Wedge w1, Wedge w2){
    printf("    Comparing wedges of ID %d and %d\n", w1->wedgeID, w2->wedgeID);

    if (w1->firstTile.x != w2->firstTile.x || w1->firstTile.y != w2->firstTile.y) {
        return 0;
    } else if (w1->segmentLength != w2->segmentLength) {
        return 0;
    } else {
        for (unsigned int blockingBits = 0; blockingBits < getMaxBlockingBits(w1); blockingBits++) {
            if (!cmEquivalent(wdi, w1->childMap[blockingBits], w2->childMap[blockingBits])) {
                return 0;
            }
        }
    }
    return 1;
}

static int cmEquivalent(WedgeDict wdi, int * cm1, int * cm2) {
    printf("      Comparing childmaps (ptrs %p and %p) for equivalence!\n", cm1, cm2);
    if (cm1[0] != cm2[0]) {
        return 0;
    } else {
        for (int k = 1; k <= cm1[0]; k++) {
            if (wdiLookupIndex(wdi, cm1[k])->mergedID != wdiLookupIndex(wdi, cm2[k])->mergedID) {
                return 0;
            }
        }
    }
    return 1;
}

///

Wedge wdiLookup(Octant oct, WedgeDict self, wedgeSpec spec) {
    WDRadix radix = self->byDiagIDs[spec.diagID];

    int hash = hashSpec(spec);
    for (int cur = hash % radix->size; ; cur = (cur+1) % radix->size) {
        if (radix->byHashes[cur] == NULL) {
            radix->byHashes[cur] = wCreate(oct, self, spec);
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

int * wslToWedges (Octant oct, WedgeDict wdi, WedgeSpecList wsl) {
    int * output = malloc(sizeof(int) * (wslLength(wsl)+1) );
    output[0] = 0;
    WedgeSpecList old = wsl;
    while (wsl != NULL) {
        Wedge w = wdiLookup(oct, wdi, wsl->data);
        output[++output[0]] = w->wedgeID;
        old = wsl;
        wsl = wsl->next;
        free(old);
    }

    return output;
}

void wRecursiveTraverse(Octant oct, WedgeDict wdi, int maxDepth) {
    for (int wedgeID = 0; wedgeID < wdi->nWedges; wedgeID++) {
        wTraverse(oct, wdi, wdiLookupIndex(wdi, wedgeID), maxDepth);
    }
    printf("DONE recursive traverse to depth %d\n", maxDepth);
}

static void wTraverse(Octant oct, WedgeDict wdi, Wedge w, int maxDepth) {
    printf("TRAVERSING: ");
    wPrint(w);
    printf("\n");

    if (w->diagID >= maxDepth) {
        printf("  -- Wedge at diag %d, too far!  Got wsl (nothing) for all children!\n", w->diagID);
    }

    unsigned int maxBlockingBits = getMaxBlockingBits(w);
    for (unsigned int blockingBits = 0; blockingBits < maxBlockingBits; blockingBits++) {
        if (w->diagID >= maxDepth) {
            w->childMap[blockingBits] = wslToWedges(oct, wdi, NULL);
        } else {
            wInnerTraverse(oct, wdi, w, blockingBits);
        }
    }
    printf("  DONE traverse of %d\n", w->wedgeID);
}

static void wInnerTraverse(Octant oct, WedgeDict wdi, Wedge w, unsigned int blockingBits) {
    printf("  -- BlockingBits %x\n", blockingBits);
    WedgeSpecList wsl = wBitsToChildSpecs(w, blockingBits);
    printf("  -- Got wsl <|");
    wslPrint(wsl);
    printf("|>\n");
    w->childMap[blockingBits] = wslToWedges(oct, wdi, wsl);
}

static WedgeSpecList wBitsToChildSpecs(Wedge wedge, unsigned int blockingBits) {
    WedgeSpecList output = NULL;

    int curBlockingStreak = blockingBits & 1;

    ray cwEdge = wedge->cwEdge;

    xyPos curTile = wedge->firstTile;

    for (int k = wedge->segmentLength; k > 0; k--) {
        if(curBlockingStreak) {
            if (blockingBits & 1) {
                // do nothing - we're on a blocking streak!
            } else {
                curBlockingStreak = 0;
                cwEdge = tileToCWRay(curTile);
            }
        } else {
            if (blockingBits & 1) {
                curBlockingStreak = 1;

                ray ccwEdge;
                ccwEdge = tileToCWRay(curTile);

                output = wslPush(output, wsCreate(wedge->diagID+1, cwEdge, ccwEdge));
            } else {
                // do nothing - we remain on a non-blocking streak!
            }
        }
        blockingBits = blockingBits >> 1;
        curTile.x--;
        curTile.y++;
    }

    if (!curBlockingStreak) {
        output = wslPush(output, wsCreate(wedge->diagID+1, cwEdge, wedge->ccwEdge));
    }

    return output;
}

static WedgeSpecList wslPush(WedgeSpecList self, wedgeSpec data) {
    WedgeSpecList new = malloc(sizeof(struct wedgeSpecList));
    new->data = data;
    new->next = self;
    return new;
}

static int wslLength(WedgeSpecList self) {
    if (self == NULL) {
        return 0;
    } else {
        return 1+wslLength(self->next);
    }
}

/*
static void wslSplit(WedgeSpecList self, ray splitEdge) {
    if (self == NULL) {
        return;
    } else if (wsContains(self->data, splitEdge)) {
        WedgeSpecList new = malloc(sizeof(struct wedgeSpecList));
        new->data = self->data;
        new->next = self->next;
        self->next = new;

        self->data.cwEdge = splitEdge;
        new->data.ccwEdge = splitEdge;
        return;
    } else {
        return wslSplit(self->next, splitEdge);
    }
}


static WedgeSpecList wslClip(WedgeSpecList self, wedgeSpec bounding) {
//    Not yet implemented!
    return NULL;
    WedgeSpecList dest = NULL;
    while (1) {
        if ( wsContains(self->data, bounding.cwEdge) ) {
            if ( wsContains(self->data, bounding.ccwEdge) ) {
                // clip entirely?
//                self->data
            } else {
            }
        } else {
            if ( wsContains(self->data, bounding.ccwEdge) ) {
            } else {
                dest = wslPush(dest, self->data);
            }
        }

    }
}
*/

void wPrint(Wedge self) {
    printf("Wedge #%d (%d|", self->wedgeID, self->diagID);
    rayPrint(self->cwEdge);
    printf("-");
    rayPrint(self->ccwEdge);
}

void wsPrint(wedgeSpec ws) {
    printf("WS (%d|", ws.diagID);
    rayPrint(ws.cwEdge);
    printf("-");
    rayPrint(ws.ccwEdge);
    printf(")");
}

void wslPrint(WedgeSpecList self) {
    while (self != NULL) {
        wsPrint(self->data);
        self = self->next;
    }
}
