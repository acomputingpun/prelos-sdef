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

// ---

typedef struct wedge * Wedge;
struct wedge {
    int wedgeID;

    int diagID;
    ray cwEdge;
    ray ccwEdge;

    xyPos firstTile;
    int segmentLength;

    int * childMap;
};

typedef struct wedgeList * WedgeList;
typedef struct wedgeSpecList * WedgeSpecList;

typedef struct wedgeDict * WedgeDict;
WedgeDict wdiCreate(Octant oct);
void wdiDestroy(WedgeDict self);
void wdiPrint(WedgeDict self);

Wedge wdiLookup(WedgeDict self, wedgeSpec spec);
void wPrint(Wedge self);

WedgeSpecList owBitsToChildren(Wedge wedge, unsigned int blockingBits);
