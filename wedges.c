#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "poses.h"
#include "rays.h"
#include "tiles.h"
#include "wedges.h"

#define wedgeCWTile(wedge) (wedge->cwEdge)
#define wedgeCCWTile(wedge) (wedge->ccwEdge)

#define hashSpec(spec) (hashRay(spec.cwEdge) ^ (3*hashRay(spec.ccwEdge)))

typedef struct wdRadix * WDRadix;

struct wedgeList {
    Wedge data;
    WedgeList next;
};

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

    self->childMap = malloc(sizeof(int *) * (1 << self->segmentLength));
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
    self->size = (1 << oct->diags[diagID].size) * (diagID+1);
    self->byHashes = malloc(sizeof(Wedge) * self->size);
    for (int k = 0; k < self->size; k++) {
        self->byHashes[k] = NULL;
    }
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
    printf("Radix of size %d:", self->size);
    for (int cur = 0; cur < self->size; cur++) {
        if (self->byHashes[cur] != NULL) {
            printf("x");
        } else {
            printf("!");
        }
    }
    printf("\n");
    for (int cur = 0; cur < self->size; cur++) {
        if (self->byHashes[cur] != NULL) {
            printf("  ");
            wPrint(self->byHashes[cur]);
            printf("\n");
        }
    }
}

Wedge wdiLookup(Octant oct, WedgeDict self, wedgeSpec spec) {
    WDRadix radix = self->byDiagIDs[spec.diagID];

    int hash = hashSpec(spec);
    for (int cur = hash % radix->size; ; cur = (cur+1) % radix->size) {
        if (radix->byHashes[cur] == NULL) {
            radix->byHashes[cur] = wCreate(oct, self, spec);
            return radix->byHashes[cur];
        } else if (wEqualsSpec(radix->byHashes[cur], spec)) {
            return radix->byHashes[cur];
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

    unsigned int maxBlockingBits = 1 << w->segmentLength;
    for (unsigned int blockingBits = 0; blockingBits < maxBlockingBits; blockingBits++) {
        if (w->diagID >= maxDepth) {
            printf("  -- Wedge at diag %d, too far!\n", w->diagID);
            printf("  -- Got wsl (nothing)\n");
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

WedgeList wlPush(WedgeList self, Wedge data) {
    WedgeList new = malloc(sizeof(struct wedgeList));
    new->data = data;
    new->next = self;
    return new;
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
