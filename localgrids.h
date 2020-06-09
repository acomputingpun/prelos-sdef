void * gridCreate(xyPos size);
void gridDestroy(void * grid);

void gridPrint(void * grid);
void tSet(void * grid, xyPos xy, char data);
void tDiagSet(void * grid, xyPos source, int len, char data);
