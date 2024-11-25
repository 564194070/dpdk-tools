
#include "../header/protocol/udp.h"



int udpApp(__attribute__((unused))  void *arg)
{
    // UDP的管道符
    int connfd = nsocket(AF_INET,SOCK_DGRAM,0);
    if (connfd == -1)
    {
        printf("udp create fs failed! \n");
        return -1;
    }

    // 收发双方地址
    struct sockaddr_in localaddr, clientaddr;
    memset(&localaddr, 0, sizeof(struct sockaddr_in));

    // 设置本机地址端口协议
    localaddr.sin_port = htons(8889);
	localaddr.sin_family = AF_INET;
    localaddr.sin_addr.s_addr = inet_addr("10.6.140.50");

    // 开始监听端口
    nbind(connfd, (struct sockaddr*)&localaddr, sizeof(localaddr));

    // 收发UDP报文缓冲区
    char buffer[UDP_APP_RECV_BUFFER_SIZE] = {0};
    socklen_t addrlen = sizeof(clientaddr);


    // 无休止的处理消息
    while (1) 
    {
        if (nrecvfrom(connfd,buffer,UDP_APP_RECV_BUFFER_SIZE,0,(struct sockaddr*)&clientaddr, &addrlen) < 0)
        {
            // 成功接收到消息
        }
        else
        {
            // 接收消息失败
        }
    }

    // 关闭套接字
    nclose(connfd);
}




int recvUDP(struct rte_mbuf *udpmbuf)
{
    struct rte_ipv4_hdr *iphdr = rte_pktmbuf_mtod_offset(udpmbuf,struct rte_ipv4_hdr*,sizeof(struct rte_ether_hdr));

    struct rte_udp_hdr *udphdr = (struct rte_udp_hdr *)(iphdr+1);
}