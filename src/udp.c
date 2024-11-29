
#include "../header/protocol/udp.h"

void udpBitMapInit()
{
    // 计算UDP BITMAP占用字节数
    uint32_t bitMapSize = rte_bitmap_get_memory_footprint(UDP_FD_BITS);

    // 申请对应数量内存，并填充0
    mem = rte_zmalloc("udp_bit_map", bitMapSize, RTE_CACHE_LINE_SIZE);
    if (mem == NULL)
    {
        printf("failed to create udp bitmap memory");
        exit(EXIT_FAILURE);
    }

    // 初始化bitMap
    udp_bitmap = rte_bitmap_init(UDP_FD_BITS, mem, bitMapSize);
    if (udp_bitmap == NULL)
    {
        printf("Failed to init bitmap!");
        exit(EXIT_FAILURE);
    }
    return;
}

void udpBitMapDestory()
{
    rte_bitmap_free(udp_bitmap);
    rte_free(mem);
}

int getFdBelongToUDPBitMap()
{
    uint32_t bitMapSize = rte_bitmap_get_memory_footprint(UDP_FD_BITS);
    for (int index = 0; index < bitMapSize; ++index)
    {
        if (!rte_bitmap_get(udp_bitmap, index))
        {
            return index;
        }
    }
    return -1;
}

struct connectConfig *getConnectConfigFromFd(int fd)
{
    struct connectConfig *host;

    for (host = headUDPConfig; host != NULL, host = host->next)
    {
        if (host->fd == fd)
        {
            return host;
        }
    }
    retur NULL;
}

int nsocket(__attribute__((unused)) int domain, int type, __attribute__((unused)) int protocol)
{
    int fd;
    fd = getFdBelongToUDPBitMap();

    struct connectConfig *host = rte_malloc("connectConfig", sizeof(struct connectConfig), 0);
    if (host == NULL)
    {
        return -1;
    }
    memset(host, 0, sizeof(struct connectConfig));

    host->fd = fd;

    if (type == SOCK_DGRAM)
    {
        host->protocol = IPPROTO_UDP;
    }

    // 创建当前应用收发缓冲区
    host->rcvbuf = rte_ring_create("udp  recv", UDPRECVSIZE, rte_socket_id(), RING_F_SP_ENQ | RING_F_SC_DEQ);
    if (host->rcvbuf == NULL)
    {

        rte_free(host);
        return -1;
    }

    host->sndbuf = rte_ring_create("udp  recv", UDPRECVSIZE, rte_socket_id(), RING_F_SP_ENQ | RING_F_SC_DEQ);
    if (host->sndbuf == NULL)
    {

        rte_ring_free(host->rcvbuf);

        rte_free(host);
        return -1;
    }

    pthread_cond_t blank_cond = PTHREAD_COND_INITIALIZER;
    rte_memcpy(&host->cond, &blank_cond, sizeof(pthread_cond_t));

    pthread_mutex_t blank_mutex = PTHREAD_MUTEX_INITIALIZER;
    rte_memcpy(&host->mutex, &blank_mutex, sizeof(pthread_mutex_t));

    LL_ADD(host, headUDPConfig);
    return fd;
}

int nbind(int fd, const struct sockaddr *addr, __attribute__((unused)) socklen_t addrlen)
{
    struct connectConfig *host = getConnectConfigFromFd(fd);
    if (host == NULL)
    {
        return -1;
    }

    const struct sockaddr_in *laddr = (const struct sockaddr_in *)addr;
    host->localport = laddr->sin_port;
	rte_memcpy(&host->localip, &laddr->sin_addr.s_addr, sizeof(uint32_t));
	rte_memcpy(host->localmac, g_src_mac, RTE_ETHER_ADDR_LEN);

    return 0;
}

int udpApp(__attribute__((unused)) void *arg)
{
    // UDP的管道符
    int connfd = (AF_INET, SOCK_DGRAM, 0);
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
    nbind(connfd, (struct sockaddr *)&localaddr, sizeof(localaddr));

    // 收发UDP报文缓冲区
    char buffer[UDP_APP_RECV_BUFFER_SIZE] = {0};
    socklen_t addrlen = sizeof(clientaddr);

    // 无休止的处理消息
    while (1)
    {
        if (nrecvfrom(connfd, buffer, UDP_APP_RECV_BUFFER_SIZE, 0, (struct sockaddr *)&clientaddr, &addrlen) < 0)
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

// 从缓冲区获取数据包信息
ssize_t nrecvfrom(int sockfd, void *buf, size_t len, __attribute__((unused))  int flags, struct sockaddr *src_addr, __attribute__((unused))  socklen_t *addrlen)
{
    
}

int recvUDP(struct rte_mbuf *udpmbuf)
{
    struct rte_ipv4_hdr *iphdr = rte_pktmbuf_mtod_offset(udpmbuf, struct rte_ipv4_hdr *, sizeof(struct rte_ether_hdr));

    struct rte_udp_hdr *udphdr = (struct rte_udp_hdr *)(iphdr + 1);
}