
#include  "../header/ring.h"

static struct inout_ring *rInst = NULL;


struct inout_ring *ringInstance(void) 
{
	if (rInst == NULL) {

		rInst = (struct inout_ring*)rte_malloc("in/out ring", sizeof(struct inout_ring), 0);
		memset(rInst, 0, sizeof(struct inout_ring));
	}
	return rInst;
}