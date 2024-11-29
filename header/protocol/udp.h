#ifndef __UDP_H_
#define __UDP_H_

#include <rte_udp.h>
#include <rte_icmp.h>
#include <rte_arp.h>
#include <rte_ether.h>
#include <rte_mbuf.h>
#include <rte_ip.h>
#include <stdatomic.h>
#include <rte_bitmap.h>
#include <rte_malloc.h>
#include "../connectConfig.h"
#include "../globalConfig.h"


#define UDPRECVSIZE 1024
#define UDPSENDSIZE 1024


#define UDP_APP_RECV_BUFFER_SIZE	128
#define UDP_FD_BITS 8

#define LL_ADD(item, list) do {		\
	item->prev = NULL;				\
	item->next = list;				\
	if (list != NULL) list->prev = item; \
	list = item;					\
} while(0)


static struct connectConfig *headUDPConfig = NULL;

static void *mem;
static struct rte_bitmap *udp_bitmap;

// UDP  BitMap初始化
void udpBitMapInit();
// UDP BitMap销毁
void udpBitMapDestory();

// UDP API接口
// 创建一个套接字
int nsocket(__attribute__((unused)) int domain, int type, __attribute__((unused))  int protocol);
// 监听端口
int nbind(int sockfd, const struct sockaddr *addr,__attribute__((unused))  socklen_t addrlen);
// 从套接字缓冲区接收数据
ssize_t nrecvfrom(int sockfd, void *buf, size_t len, __attribute__((unused))  int flags, struct sockaddr *src_addr, __attribute__((unused))  socklen_t *addrlen);
//  向套接字缓冲区发送数据




//UDP服务
int udpApp(__attribute__((unused))  void *arg);

// 接收UDP报文
int recvUDP(struct rte_mbuf *udpmbuf);

int getFdBelongToUDPBitMap();

#endif