#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "poses.h"
#include "rays.h"
#include "tiles.h"
#include "wedges.h"
#include "wdicts.h"

#define wedgeCWTile(wedge) (wedge->cwEdge)
#define wedgeCCWTile(wedge) (wedge->ccwEdge)

#define hashSpec(spec) ((37 * hashRay(spec.cwEdge)) ^ (101*hashRay(spec.ccwEdge)))

struct wedgeSpecList {
    wedgeSpec data;
    WedgeSpecList next;
};
static WedgeSpecList wslPush(WedgeSpecList self, wedgeSpec data);

static WedgeSpecList wBitsToChildSpecs(Wedge wedge, unsigned int blockingBits);
static int wslLength(WedgeSpecList self);
static void wInnerTraverse(Octant oct, WedgeDict wdi, Wedge w, unsigned int blockingBits);

int wEqualsSpec(Wedge self, wedgeSpec spec) {
    return rayEquals(self->cwEdge, spec.cwEdge) && rayEquals(self->ccwEdge, spec.ccwEdge) && (self->diagID == spec.diagID);
}

Wedge wCreate(Octant oct, WedgeDict wdi, wedgeSpec spec) {
    Wedge self = malloc(sizeof(struct wedge));
    self->cwEdge = spec.cwEdge;
    self->ccwEdge = spec.ccwEdge;
    self->diagID = spec.diagID;

    self->firstTile = cwDiagIntersectTile( self->cwEdge, self->diagID);
    xyPos lastTile = ccwDiagIntersectTile( self->ccwEdge, self->diagID);
    self->segmentLength = (self->firstTile.x - lastTile.x) + 1;

    self->wedgeID = wdiNextWedgeID(wdi);
    self->mergedID = self->wedgeID;

    self->childMap = malloc(sizeof(int *) * getMaxBlockingBits(self) );
    return self;
}
void wDestroy(Wedge self) {
    free(self);
}

///

static int cmEquivalent(WedgeDict wdi, int * cm1, int * cm2);

int wEquivalent(WedgeDict wdi, Wedge w1, Wedge w2){
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

void wTraverse(Octant oct, WedgeDict wdi, Wedge w, int maxDepth) {
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
