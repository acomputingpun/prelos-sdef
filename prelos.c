#include "poses.h"
#include "tiles.h"
#include "rays.h"
#include "wedges.h"

void test() {
    Octant o = octCreate(5);
    octPrint(o);

    WedgeDict wdi = wdiCreate(o);
    wdiPrint(wdi);

    Wedge w = wdiLookup(wdi, wsInitial());

    wdiPrint(wdi);

    owBitsToChildren(w, 0);

    wdiPrint(wdi);

    return;
}

int main (int argc, char** argv) {
    test();
    return 0;
}
