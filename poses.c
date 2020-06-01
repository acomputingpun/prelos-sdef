#include <stdio.h>
#include "poses.h"

#define N_PRIMES 12
int PRIMES[N_PRIMES] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37};

xyPos gcdReduce(xyPos xy) {
    for (int k = 0; k < N_PRIMES; ) {
        int curPrime = PRIMES[k];
        if (xy.x % curPrime == 0 && xy.y % curPrime == 0) {
            xy.x = xy.x / curPrime;
            xy.y = xy.y / curPrime;
        } else {
            k++;
        }
    }
    return xy;
}
void xyPosPrint(xyPos xy) {
    printf("(%d, %d)", xy.x, xy.y);
}
