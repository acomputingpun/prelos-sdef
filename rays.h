typedef xyPos ray;

#define rayPrint(r) xyPosPrint(r)
#define rayEquals(r1, r2) ((r1.x == r2.x) && (r1.y == r2.y))
//#define hashRay(r) ( (r.x ^ (r.x*7)) + (r.y ^ (r.y*11)) )
#define hashRay(r) (r.x ^ (r.y << 8))

#define rayCmp(r1, r2) xyGradCmp(r1, r2)

static inline ray rayCreate(int x, int y) {
    ray r;
    r.x = x;
    r.y = y;
    return r;
}

static inline xyPos cwDiagIntersectTile(ray edge, int xAxisIntersect) {
    xyPos intersect;
    intersect.x = floor(xAxisIntersect / (1.0 + ((float)edge.y / (float)edge.x)));
    intersect.y = xAxisIntersect - intersect.x;
    return intersect;
}

static inline xyPos ccwDiagIntersectTile(ray edge, int xAxisIntersect) {
    xyPos intersect;
    intersect.x = ceil(xAxisIntersect / (1.0 + ((float)edge.y / (float)edge.x)));
    intersect.y = xAxisIntersect - intersect.x;
    return intersect;
}
