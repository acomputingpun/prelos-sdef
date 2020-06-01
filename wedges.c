#include <stdio.h>
#include <stdlib.h>

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
    int size;
    WDRadix * byDiagIDs;
};
struct wdRadix {
    int size;
    Wedge * byHashes;
};


static Wedge wCreate(wedgeSpec spec);
static void wDestroy(Wedge self);

static WDRadix wdiRadixCreate(Octant oct, int diagID);
static void wdiRadixDestroy(WDRadix self);
static void wdiRadixPrint(WDRadix self);
static Wedge _wdiRadixLookup(WDRadix self, wedgeSpec spec);

static int wEqualsSpec(Wedge self, wedgeSpec spec) {
    return rayEquals(self->cwEdge, spec.cwEdge) && rayEquals(self->ccwEdge, spec.ccwEdge) && (self->diagID == spec.diagID);
}

static Wedge wCreate(wedgeSpec spec) {
    Wedge self = malloc(sizeof(struct wedge));
    self->cwEdge = spec.cwEdge;
    self->ccwEdge = spec.ccwEdge;
    self->diagID = spec.diagID;
    return self;
}

static void wDestroy(Wedge self) {
    free(self);
}

WedgeDict wdiCreate(Octant oct) {
    WedgeDict self = malloc(sizeof(struct wedgeDict));
    self->size = oct->nDiags;
    self->byDiagIDs = malloc(sizeof(WDRadix) * self->size);
    for (int diagID = 0; diagID < self->size; diagID++) {
        self->byDiagIDs[diagID] = wdiRadixCreate(oct, diagID);
    }
    return self;
}
void wdiDestroy(WedgeDict self) {
    for (int diagID = 0; diagID < self->size; diagID++) {
        wdiRadixDestroy(self->byDiagIDs[diagID]);
    }
    free(self->byDiagIDs);
    free(self);
}
void wdiPrint(WedgeDict self) {
    printf("WedgeDict with %d diag radixes\n", self->size);
    for (int diagID = 0; diagID < self->size; diagID++) {
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

Wedge wdiLookup(WedgeDict self, wedgeSpec spec) {
    WDRadix radix = self->byDiagIDs[spec.diagID];
    return _wdiRadixLookup(radix, spec);
}

static Wedge _wdiRadixLookup(WDRadix self, wedgeSpec spec) {
    int hash = hashSpec(spec);
    for (int cur = hash % self->size; ; cur = (cur+1) % self->size) {
        if (self->byHashes[cur] == NULL) {
            self->byHashes[cur] = wCreate(spec);
            return self->byHashes[cur];
        } else if (wEqualsSpec(self->byHashes[cur], spec)) {
            return self->byHashes[cur];
        }
    }
    return NULL;
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

//void wTraverse(Octant )

WedgeSpecList owBitsToChildren(Wedge wedge, unsigned int blockingBits) {
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

static void wslClip(WedgeSpecList self, wedgeSpec bounding) {
    return;
}

void wPrint(Wedge self) {
    printf("Wedge (%d|", self->diagID);
    rayPrint(self->cwEdge);
    printf("-");
    rayPrint(self->ccwEdge);
}
