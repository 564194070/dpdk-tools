#ifndef __RING_H_
#define __RING_H_




#include <rte_ring.h>
#include <rte_malloc.h>
#include <string.h>


#define RING_SIZE	1024


struct inout_ring {

	struct rte_ring *in;
	struct rte_ring *out;
};

struct inout_ring *ringInstance(void); 


#endif