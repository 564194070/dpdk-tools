#ifndef __ICMP_H__
#define __ICMP_H__

#include <rte_malloc.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_icmp.h>

#include "../globalConfig.h"

// 发送icmp数据包
struct rte_mbuf *send_icmp_pack(struct rte_mempool *mbuf_pool, uint8_t *dst_mac, uint32_t sip, uint32_t dip, uint16_t id, uint16_t seqnb, uint8_t* data, uint16_t data_len);

// 构建icmp数据包
int build_icmp_packet(uint8_t *msg, uint8_t *dst_mac, uint32_t sip, uint32_t dip, uint16_t id, uint16_t seqnb, uint8_t* data, uint8_t data_len);
uint16_t ng_checksum(uint16_t *addr, int count);

#endif