#include "poses.h"
#include "grids.h"

void * gridCreate(void * callbackFunc) {
    return callbackFunc;
}

int tLookup(void * grid, xyPos xy) {
    return tDiagLookups(grid, xy, 1);
}

unsigned int tDiagLookups(void * grid, xyPos source, int len) {
    int (*callback)(int x, int y, int segmentLen) = grid;
    return callback(source.x, source.y, len);
}
