typedef struct wedgeSpec wedgeSpec;
struct wedgeSpec {
    int diagID;
    ray cwEdge;
    ray ccwEdge;
};

static inline wedgeSpec wsCreate(int diagID, ray cwEdge, ray ccwEdge) {
    wedgeSpec self;
    self.diagID = diagID;
    self.cwEdge = gcdReduce(cwEdge);
    self.ccwEdge = gcdReduce(ccwEdge);
    return self;
}
static inline wedgeSpec wsInitial(){
    return wsCreate(0, rayCreate(1,0), rayCreate(1,1) );
}
static inline int wsContains(wedgeSpec ws, ray edge){
    if (rayCmp(ws.ccwEdge, edge) < 0) {
        return 0;
    } else if (rayCmp(ws.cwEdge, edge) > 0) {
        return 0;
    }
    return 1;
}
static inline int wsContainsInclusive(wedgeSpec ws, ray edge) {
    if (rayCmp(ws.ccwEdge, edge) <= 0) {
        return 0;
    } else if (rayCmp(ws.cwEdge, edge) >= 0) {
        return 0;
    }
    return 1;
}

#define hashSpec(spec) ((37 * hashRay(spec.cwEdge)) ^ (101*hashRay(spec.ccwEdge)))

// ---

struct wedge {
    int wedgeID;

    int diagID;
    ray cwEdge;
    ray ccwEdge;

    xyPos firstTile;
    int segmentLength;
    ccMask cornerClips;

    int ** childMap;
    int mergedID;
    int finalNodeID;
};

typedef struct wedge * Wedge;
typedef struct wedgeDict * WedgeDict;

Wedge wCreate(Octant oct, WedgeDict wdi, wedgeSpec spec);
void wPrint(Wedge self);
void wDestroy(Wedge self);

int wEquivalent(WedgeDict wdi, Wedge w1, Wedge w2);
int wEqualsSpec(Wedge self, wedgeSpec spec);

void wTraverse(Octant oct, WedgeDict wdi, Wedge w);

void wsPrint(wedgeSpec ws);
