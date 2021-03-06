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

typedef struct wedgeSpecList * WedgeSpecList;
struct wedgeSpecList {
    wedgeSpec data;
    WedgeSpecList next;
};

static WedgeSpecList wslPush(WedgeSpecList self, wedgeSpec data);
static void wslDestroy(WedgeSpecList self);
static void wslPrint(WedgeSpecList self);

static WedgeSpecList wBitsToChildSpecs(Wedge wedge, unsigned int blockingBits);
static int wslLength(WedgeSpecList self);
static int * wslToWedges (Octant oct, WedgeDict wdi, WedgeSpecList wsl);
static void wslSplit(WedgeSpecList self, ray splitEdge);

static void wInnerTraverse(Octant oct, WedgeDict wdi, Wedge w, unsigned int blockingBits);


static WedgeSpecList wslPush(WedgeSpecList self, wedgeSpec data) {
    WedgeSpecList new = malloc(sizeof(struct wedgeSpecList));
    new->data = data;
    new->next = self;
    return new;
}

static void wslDestroy(WedgeSpecList self) {
    if (self != NULL) {
        wslDestroy(self->next);
        free(self);
    }
}
static void wslPrint(WedgeSpecList self) {
    while (self != NULL) {
        wsPrint(self->data);
        self = self->next;
    }
}
static int wslLength(WedgeSpecList self) {
    if (self == NULL) {
        return 0;
    } else {
        return 1+wslLength(self->next);
    }

}

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
    self->cornerClips = 0;
    if ( rayCmp(self->cwEdge, self->firstTile) > 0 ) {
        self->cornerClips |= 1;
    }
    if (rayCmp(lastTile, self->ccwEdge) > 0) {
        self->cornerClips |= 2;
    }

    self->wedgeID = wdiNextWedgeID(wdi);
    self->mergedID = self->wedgeID;

    self->childMap = malloc(sizeof(int *) * getMaxBlockingBits(self) );
    return self;
}
void wDestroy(Wedge self) {
    unsigned int maxBlockingBits = getMaxBlockingBits(self);
    for (int blockingBits = 0; blockingBits < maxBlockingBits; blockingBits++) {
        free(self->childMap[blockingBits]);
    }
    free(self->childMap);
    free(self);
}

///

static int cmEquivalent(WedgeDict wdi, int * cm1, int * cm2);

int wEquivalent(WedgeDict wdi, Wedge w1, Wedge w2){
    if (w1->firstTile.x != w2->firstTile.x || w1->firstTile.y != w2->firstTile.y) {
        return 0;
    } else if (w1->segmentLength != w2->segmentLength) {
        return 0;
    } else if (w1->cornerClips != w2->cornerClips) {
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

static int * wslToWedges (Octant oct, WedgeDict wdi, WedgeSpecList wsl) {
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

void wTraverse(Octant oct, WedgeDict wdi, Wedge w) {
    int maxDepth = oct->nDiags -1;

    unsigned int maxBlockingBits = getMaxBlockingBits(w);
    for (unsigned int blockingBits = 0; blockingBits < maxBlockingBits; blockingBits++) {
        if (w->diagID >= maxDepth) {
            w->childMap[blockingBits] = wslToWedges(oct, wdi, NULL);
        } else {
            wInnerTraverse(oct, wdi, w, blockingBits);
        }
    }
}

static void wInnerTraverse(Octant oct, WedgeDict wdi, Wedge w, unsigned int blockingBits) {
    WedgeSpecList wsl = wBitsToChildSpecs(w, blockingBits);

    if ((w->diagID+1) % oct->autoDividePeriod == 0) {
        diagonal diag = (oct->diags[w->diagID]);

        int nSplitRays = 1 << (w->diagID / oct->autoDividePeriod);
        int step = diag.size / (nSplitRays);
        xyPos splitTile = oct->tilePoses[diag.firstTile];
        for (int k = 1; k < nSplitRays; k += 2) {
            splitTile.x -= step;
            splitTile.y += step;
            ray splitRay = tileToCWRay(splitTile);
            wslSplit(wsl, splitRay);
            splitTile.x -= step;
            splitTile.y += step;
        }
    }

    w->childMap[blockingBits] = wslToWedges(oct, wdi, wsl);
    if (0) {
        wslDestroy(wsl);
    }
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
    } else {
        wslSplit(self->next, splitEdge);
    }
}

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
