#include <stdlib.h>

#include "poses.h"
#include "grids.h"
#include "localgrids.h"

struct grid {
    xyPos size;
    char * matrix;
};
typedef struct grid * Grid;

int tLookup(void * grid, xyPos xy) {
    Grid self = (Grid) grid;
    return self->matrix[self->size.x * xy.y + xy.x];
}

unsigned int tDiagLookups(void * grid, xyPos source, int len) {
    unsigned int result = 0;
    for (int k = 0; k < len; k++) {
        result = result << 1;
        if (tLookup(grid, source)) {
            result++;
        }
        source.x--;
        source.y++;
    }
    return result;
}

unsigned int tLookups(void * grid, xyPos * poses, int len) {
    unsigned int result = 0;
    for (int k = 0; k < len; k++) {
        result = result << 1;
        if (tLookup(grid, poses[k])) {
            result++;
        }
    }
    return result;
}

void * gridCreate(xyPos size) {
    Grid self = malloc(sizeof(struct grid));
    self->matrix = malloc(sizeof(char) * size.x * size.y);
    return (void *) self;
}
void gridDestroy(void * grid) {
    Grid self = grid;
    free(self->matrix);
    free(self);
}
