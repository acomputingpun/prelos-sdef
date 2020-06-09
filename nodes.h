typedef struct nodeMemory * NodeMemory;

NodeMemory nmCreate(WedgeDict wdi);
void nmDestroy(NodeMemory nm);
void nmPrint(NodeMemory nm);

void nmgRecast(NodeMemory nm, void * grid);
void nmgRecastFrom(NodeMemory nm, void * grid, int nodeID);

NodeMemory nmUnflatten(FILE * stream);
void nmFlatten(NodeMemory nm, FILE * stream);
