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
#include <arpa/inet.h>
#include "../ring.h"
#include "../connect.h"
#include "../globalConfig.h"
#include "./arp.h"

#define UDPRECVSIZE 1024
#define UDPSENDSIZE 1024

#define UDP_APP_RECV_BUFFER_SIZE 128
#define UDP_FD_BITS 8

// UDP  BitMap初始化
void udpBitMapInit(void);
// UDP BitMap销毁
void udpBitMapDestory(void);



// 构建UDP报文
int build_udp_pack(uint8_t* msg, uint32_t sip, uint32_t dip, uint16_t sport, uint16_t dport, uint8_t *srcmac, uint8_t *dstmac, unsigned char *data, uint16_t total_len);
// 发送UDP报文
struct rte_mbuf *send_udp_pack(struct rte_mempool *mbuf_pool, uint32_t sip, uint32_t dip, uint16_t sport, uint16_t dport, uint8_t *srcmac, uint8_t *dstmac, uint8_t *data, uint16_t length);



// UDP API接口
// 创建一个套接字
int nsocket(__attribute__((unused)) int domain, int type, __attribute__((unused)) int protocol);
// 监听端口
int nbind(int sockfd, const struct sockaddr *addr, __attribute__((unused)) socklen_t addrlen);
// 从套接字缓冲区接收数据
ssize_t nrecvfrom(int sockfd, void *buf, size_t len, __attribute__((unused)) int flags, struct sockaddr *src_addr, __attribute__((unused)) socklen_t *addrlen);
//  向套接字缓冲区发送数据
ssize_t nsendto(int sockfd, const void *buf, size_t len, __attribute__((unused))  int flags, const struct sockaddr *dest_addr, __attribute__((unused))  socklen_t addrlen);

int nclose(int fd);

// UDP服务
int udpApp(__attribute__((unused)) void *arg);
// 接收UDP报文
int recvUDP(struct rte_mbuf *udpmbuf);


int getFdBelongToUDPBitMap(void);


// send udp package to network
int sendUdpPackage(struct rte_mempool* mbuf_pool);

#endif