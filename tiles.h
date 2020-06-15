struct diagonal {
    int firstTile;
    int size;
};
typedef struct diagonal diagonal;

struct octant {
    int nDiags;
    int nTiles;
    int autoDividePeriod;
    diagonal * diags;
    xyPos * tilePoses;
};
typedef struct octant * Octant;

Octant octCreate(int nDiags, int autoDividePeriod);
void octDestroy(Octant oct);
void octPrint(Octant oct);
