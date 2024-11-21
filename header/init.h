#ifndef __INIT_H_
#define __INIT_H_

#include "globalConfig.h"

#include "ring.h"
#include "./protocol/arp.h"

#include  <rte_mbuf.h>
#include  <rte_ether.h>



int initDPDK  (int argc, char* argv[]);
void ifIndex_init (struct rte_mempool * mbuf_pool);
void timer_init(struct rte_mempool * mbuf_pool,unsigned id);
void ring_init(void);


#endif