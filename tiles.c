#include <stdio.h>
#include <stdlib.h>

#include "poses.h"
#include "tiles.h"

static void octDiagPrint(Octant self, diagonal diag);

Octant octCreate(int nDiags) {
    Octant self = malloc(sizeof(struct octant));

    self->nDiags = nDiags;
    self->diags = malloc(sizeof(struct diagonal) * nDiags);

    self->nTiles = ((nDiags+1)/2) * ((nDiags/2)+1);
    self->tilePoses = malloc(sizeof(xyPos) * self->nTiles);

    int curTile = 0;
    for (int curDiag = 0; curDiag < nDiags; curDiag++) {
        self->diags[curDiag].firstTile = curTile;
        self->diags[curDiag].size = (curDiag/2)+1;
        for (int x = curDiag; x >= curDiag-x; x--) {
            self->tilePoses[curTile].x = x;
            self->tilePoses[curTile].y = curDiag - x;
            curTile++;
        }
    }
    return self;
}

void octDestroy(Octant self) {
    free(self->tilePoses);
    free(self->diags);
    free(self);
}

void octPrint(Octant self) {
    printf("Test printing octant\n");
    printf("Octant has %d total diagonals\n", self->nDiags);
    printf("Octant has %d total tiles\n", self->nTiles);
    for (int curDiag = 0; curDiag < self->nDiags; curDiag++) {
        printf("Diag %d, size %d: ", curDiag, self->diags[curDiag].size);
        octDiagPrint(self, self->diags[curDiag]);
    }
}

static void octDiagPrint(Octant self, diagonal diag) {
    for (int k = 0; k < diag.size; k++) {
        xyPosPrint(self->tilePoses[diag.firstTile+k]);
    }
    printf("\n");
}
