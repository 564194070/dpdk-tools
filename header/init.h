#ifndef __INIT_H_
#define __INIT_H_

#include "globalConfig.h"

#include "ring.h"
#include "./protocol/arp.h"
#include "./protocol/udp.h"
#include  "../header/globalConfig.h"
#include  "../header/ring.h"
#include "../header/work.h"
#include "../header/init.h"
#include "../header/protocol/arp.h"

#include  <rte_mbuf.h>
#include  <rte_ether.h>
#include <rte_malloc.h>
#include <rte_common.h>
#include <rte_timer.h>




int initDPDK  (int argc, char* argv[]);
void ifIndex_init (struct rte_mempool * mbuf_pool);
void timer_init(struct rte_mempool * mbuf_pool);
void ring_init(void);


#endif