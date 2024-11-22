#ifndef __GLOBALCONFIG_H__
#define __GLOBALCONFIG_H__

#include <rte_ethdev.h>
#include "util.h"

//  定时器事件
// 10ms*10000*12 一分钟
#define TIMER_RESOLUTION_CYCLES 20000000000ULL
#define TIME_RESOLUTION_CYCLES 20000000000ULL



// DPDK配置内容
// 内存池中的块 4K - 1
#define NUMMBUFS (4096 - 1)
// 定义缓冲区最多接受的报文数量
#define BURSTSIZE 32
// 定义网卡ID
extern int g_dpdk_ifIndex;

// 默认配置 项目使用点运算符和成员名
extern  const struct rte_eth_conf ifIndex_conf_default;


// 本机配置
// 本机MAC地址
uint8_t g_src_mac[RTE_ETHER_ADDR_LEN];
// 本机IP地址
extern uint32_t g_src_ip;

# endif