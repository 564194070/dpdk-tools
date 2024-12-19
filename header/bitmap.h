#include <stdio.h>
#include <stdlib.h>


struct BitMap {
    unsigned int* bits;
    size_t len;
};

struct BitMap* bitMapCreate(size_t len);
void bitMapDstroy(struct BitMap* bm);
int bitMapSet(struct BitMap* bm, size_t index);
int bitMapClear(struct BitMap* bm, size_t index);

int bitMapIsSet(struct BitMap* bm, size_t index);
