#ifndef __WORK_H_
#define __WORK_H_

#include "globalConfig.h"

#include "ring.h"
#include "./protocol/arp.h"
#include "./protocol/icmp.h"

#include "./protocol/udp.h"

#include  <rte_mbuf.h>
#include  <rte_ether.h>


int pkt_process(void *arg);


#endif