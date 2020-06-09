#include <stdlib.h>
#include <stdio.h>

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
        if (tLookup(grid, source) == '#') {
            result++;
        } else {
            tSet(grid, source, 'o');
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
    self->size = size;
    self->matrix = malloc(sizeof(char) * size.x * size.y);
    return (void *) self;
}
void gridDestroy(void * grid) {
    Grid self = grid;
    free(self->matrix);
    free(self);
}

void gridPrint(void * grid) {
    Grid self = grid;

    xyPos xy;
    for (xy.y = self->size.y-1; xy.y >= 0; xy.y--) {
        for (xy.x = 0; xy.x < self->size.x; xy.x++) {

            if (tLookup(grid, xy) <= 32) {
                putchar(' ');
            } else {
                putchar(tLookup(grid, xy));
            }
        }
        putchar('\n');
    }
}

void tSet(void * grid, xyPos xy, char data) {
    Grid self = grid;

    self->matrix[self->size.x * xy.y + xy.x] = data;
}

void tDiagSet(void * grid, xyPos source, int len, char data) {
    for (int k = 0; k < len; k++) {
        tSet(grid, source, data);
        source.x--;
        source.y++;
    }
}
