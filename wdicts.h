WedgeDict wdiCreate(Octant oct);
void wdiDestroy(WedgeDict self);
void wdiPrint(WedgeDict self);

Wedge wdiLookup(Octant oct, WedgeDict self, wedgeSpec spec);
Wedge wdiLookupIndex(WedgeDict self, int wedgeID);

int wdiNextWedgeID(WedgeDict wdi);
int wdiNumWedges(WedgeDict wdi);

void wdiBuild(Octant oct, WedgeDict wdi, int maxDepth);
void wdiMergeEquivalent(WedgeDict wdi);
