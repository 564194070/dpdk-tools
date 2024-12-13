#ifndef __ARP_H__
#define __ARP_H__

#include <rte_ether.h>
#include <rte_malloc.h>
#include <rte_arp.h>
#include <rte_timer.h>

#include "../globalConfig.h"
#include "../ring.h"

// ARP表 ARP规则定义
#define ARP_ENTRY_STATUS_DYNAMIC 0
#define ARP_ENTRY_STATUS_STATIC 1


// ARP表数据结构
struct arp_entry 
{
    uint32_t ip;
    uint8_t hwaddr[RTE_ETHER_ADDR_LEN];
    // 动态静态
    uint8_t type;
    // 内存没有对齐


    // 双向链表
    struct arp_entry *next;
    struct arp_entry *prev;
};

// ARP表
struct arp_table 
{
    struct arp_entry *entries;
    int count;
};

// 添加和删除
#define LL_ADD(item, list) do { \
    item ->prev = NULL; \
    item ->next =list; \
    if (list != NULL) list->prev = item;\
    list = item; \
} while (0)

#define LL_REMOVE(item, list) do { \
    if (item->prev != NULL) item->prev->next = item->next; \
    if (item->next != NULL) item->next->prev = item->prev; \
    if (list == item) list = item->next; \
    item->prev = item->next; \
} while(0) 

struct arp_table *arp_table_instance (void);



// 获取目标MAC地址
uint8_t* get_dst_mac(uint32_t dip);


// 给定时器的定期ARP扫描
void arp_request_timer_cb (__attribute__((unused)) struct rte_timer* timer, __attribute__((unused)) void* arg);
int build_arp_packet(uint8_t *msg,uint16_t opcode ,uint8_t* dst_mac, uint32_t src_ip, uint32_t dst_ip);
struct rte_mbuf* send_arp_pack(struct rte_mempool* mbuf_pool,uint16_t opcode , uint8_t* dst_mac, uint32_t src_ip, uint32_t dst_ip);


#endif