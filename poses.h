#define xyGradCmp(p1, p2) (p1.y*p2.x - p2.y*p1.x)

struct xyPos {
    char x;
    char y;
};
typedef struct xyPos xyPos;

xyPos gcdReduce(xyPos xy);
void xyPosPrint(xyPos xy);

#define getMaxBlockingBits(w) (1 << w->segmentLength)

typedef int ccMask;
