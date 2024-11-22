#ifndef __UDP_H_
#define __UDP_H_

#include <rte_udp.h>
#include <rte_icmp.h>
#include <rte_arp.h>
#include <rte_ether.h>
#include <rte_mbuf.h>

// 接收UDP报文
int recvUDP(struct rte_mbuf *udpmbuf);

#endif