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

// ---

typedef struct wedge * Wedge;
struct wedge {
    int wedgeID;

    int diagID;
    ray cwEdge;
    ray ccwEdge;

    xyPos firstTile;
    int segmentLength;

    int ** childMap;
};

typedef struct wedgeList * WedgeList;
typedef struct wedgeSpecList * WedgeSpecList;

typedef struct wedgeDict * WedgeDict;
WedgeDict wdiCreate(Octant oct);
void wdiDestroy(WedgeDict self);
void wdiPrint(WedgeDict self);

Wedge wdiLookup(Octant oct, WedgeDict self, wedgeSpec spec);
void wPrint(Wedge self);

void wRecursiveTraverse(Octant oct, WedgeDict wdi, int maxDepth);
