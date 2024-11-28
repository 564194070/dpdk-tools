#ifndef __CONNECT_CONFIG_H_
#define __CONNECT_CONFIG_H_

#include <rte_ether.h>


struct connectConfig 
{
    // 文件描述符套接字
    int fd;

    // 链接配置
    uint32_t localip; // ip --> mac
	uint8_t localmac[RTE_ETHER_ADDR_LEN];
	uint16_t localport;
    uint8_t protocol;


    // 收发缓冲区队列
    struct rte_ring *sndbuf;
	struct rte_ring *rcvbuf;


    // 双向链表查询
	struct localhost *prev;
	struct localhost *next;

	pthread_cond_t cond;
	pthread_mutex_t mutex;

};


#endif