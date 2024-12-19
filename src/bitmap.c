#include  "../header/bitmap.h"


struct BitMap* bitMapCreate(size_t len)
{
    struct BitMap* bm = (struct BitMap*)malloc(sizeof(struct BitMap));
    if (bm == NULL)
    {
        return NULL;
    }

    

    // all bits 
    bm->len=len;
    size_t bitLen = (len+31)/32;

    bm->bits = (unsigned int*) calloc (bitLen, sizeof(unsigned int));

    if (bm->bits == NULL)
    {
        free(bm);
        return NULL;
    }

    return bm;
    
}


void bitMapDstroy(struct BitMap* bm)
{
    if (bm != NULL)
    {
        free(bm->bits);
        free(bm);
    }
    return ;
}

int bitMapSet(struct BitMap* bm, size_t index)
{
    if (index > bm->len)
    {
        return -1;
    }

    // array index
    size_t bitArrayIndex = index /32;
    // num index
    size_t bitNumIndex = index % 32;
    // create a mask and set bit = 1
    unsigned int bit_mask = (1U << bitNumIndex);
    // set bitmap = 1
    bm->bits[bitArrayIndex] |= bit_mask;
    return 0;
}

int bitMapClear(struct BitMap* bm, size_t index)
{
    if (index > bm->len)
    {
        return -1;
    }

    // array index
    size_t bitArrayIndex = index /32;
    // num index
    size_t bitNumIndex = index % 32;
    // create a mask and set bit = 1
    unsigned int bit_mask = ~(1U << bitNumIndex);
    // set bitmap = 1
    bm->bits[bitArrayIndex] &= bit_mask;
    return 0;
}


int bitMapIsSet(struct BitMap* bm, size_t index)
{
    if (index > bm->len)
    {
        return -1;
    }

    // array index
    size_t bitArrayIndex = index /32;
    // num index
    size_t bitNumIndex = index % 32;
    // create a mask and set bit = 1
    unsigned int bit_mask = (1U << bitNumIndex);
    // set bitmap = 1
    
    // !=0 bit ==1
    // 0 clear
    // 1 have
    return ((bm->bits[bitArrayIndex] & bit_mask) != 0);
}
