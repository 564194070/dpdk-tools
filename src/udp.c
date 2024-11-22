
#include "../header/protocol/udp.h"

int recvUDP(struct rte_mbuf *udpmbuf)
{
    struct rte_ether_hdr *ehdr = rte_pktmbuf_mtod(udpmbuf, struct rte_ether_hdr*);
}