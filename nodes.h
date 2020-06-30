typedef struct nodeMemory * NodeMemory;
typedef struct node * Node;

struct nodeMemory {
    int oDepth;
    int autoDividePeriod;
    int nNodes;
    Node * matrix;
};
struct node {
    xyPos firstTile;
    int segmentLength;
    int ** childMap;
};

void nmDestroy(NodeMemory nm);
void nmPrint(NodeMemory nm);

int nmDepth(NodeMemory nm);
int nmPeriod(NodeMemory nm);

void nmgRecast(NodeMemory nm, void * grid);
void nmgRecastFrom(NodeMemory nm, void * grid, int nodeID);

NodeMemory nmUnflatten(FILE * stream);
int nmFlatten(NodeMemory nm, FILE * stream);
