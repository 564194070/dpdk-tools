
#include "../header/protocol/udp.h"

struct rte_bitmap *udp_bitmap;
void *mem;

extern uint8_t g_src_mac[RTE_ETHER_ADDR_LEN];

void udpBitMapInit(void)
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

void udpBitMapDestory(void)
{
    rte_bitmap_free(udp_bitmap);
    rte_free(mem);
}

int getFdBelongToUDPBitMap(void)
{
    uint32_t bitMapSize = rte_bitmap_get_memory_footprint(UDP_FD_BITS);
    for (unsigned int index = 0; index < bitMapSize; ++index)
    {
        if (!rte_bitmap_get(udp_bitmap, index))
        {
            return index;
        }
    }
    return -1;
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


// 从缓冲区获取数据包信息
ssize_t nrecvfrom(int sockfd, void *buf, size_t len, __attribute__((unused))  int flags, struct sockaddr *src_addr, __attribute__((unused))  socklen_t *addrlen)
{
    struct connectConfig *host = getConnectConfigFromFd(sockfd);
    if (host == NULL)
    {
        return -1;
    }

    // 指向数据的指针
    unsigned char *ptr = NULL;

    struct connectData *data = NULL;
    //struct sockaddr_in *saddr = (struct sockaddr_in *)src_addr;

    //   
    int  nb =  -1;
    pthread_mutex_lock(&host->mutex);
    // 从套接字缓冲区接收数据包
    // 将数据指针交给data
    while ((nb=rte_ring_mc_dequeue(host->rcvbuf,(void **)&data)) < 0)
    {
        pthread_cond_wait(&host->cond, &host->mutex);
    }
    pthread_mutex_unlock(&host->mutex);

    // 将数据从ringbuffer移出
    if (len < data->length)
    {
        // 缓冲区太小 剩下内容返回缓冲区
        rte_memcpy(buf, data->data, len);

        ptr = rte_malloc("unsigned char *", data->length-len, 0);
        // 准备写回缓冲区的数据
        rte_memcpy(ptr, data->data+len, data->length-len);
        data->length  =  data->length - len;
        rte_free(data->data);
        
        data->data = ptr;
        rte_ring_mp_enqueue(host->rcvbuf, data);
        return len;
    }  
    else
    {
        // 缓冲区满足要求
        rte_memcpy(buf,data->data,data->length);
        int res = data->length;
        rte_free(data->data);
        rte_free(data);
        return  res;
    }
}

// 发送数据到缓冲区
ssize_t nsendto(int sockfd, const void *buf, size_t len, __attribute__((unused))  int flags, const struct sockaddr *dest_addr, __attribute__((unused))  socklen_t addrlen) 
{
    struct connectConfig  *host  = getConnectConfigFromFd(sockfd);
    if  (host == NULL)
    {
        return -1;
    }

    const struct sockaddr_in *daddr = (const struct sockaddr_in *)dest_addr;
    struct connectData *sendData = rte_malloc("data",sizeof(struct connectData),0);
    if (sendData  ==  NULL)
    {
        return-1;
    } 

    //构建发送数据包
    sendData->dip = daddr->sin_addr.s_addr;
    sendData->dport = daddr->sin_port;
    sendData->sip = host->localip;
    sendData->sport = host->localport;
    sendData->length = len;

    //  创建发送数据加入发送数据包
    sendData->data = rte_malloc("sendData",len,0 );
    if (sendData->data  == NULL)
    {
        rte_free(sendData);
        return -1;
    }

    rte_memcpy(sendData->data,buf,len);
    // sned to send cache
    rte_ring_mp_enqueue(host->sndbuf,sendData);
    return len;
}

int nclose(int fd) {

	struct connectConfig *head =  getConnectConfigFromFd(fd);
	if (head == NULL) return -1;

	LL_REMOVE(head, headUDPConfig);

	if (head->rcvbuf) {
		rte_ring_free(head->rcvbuf);
	}
	if (head->sndbuf) {
		rte_ring_free(head->sndbuf);
	}

	rte_free(head);
    return  0;
}



// 构建UDP报文
int build_udp_pack(uint8_t* msg, uint32_t sip, uint32_t dip, uint16_t sport, uint16_t dport, uint8_t *srcmac, uint8_t *dstmac, unsigned char *data, uint16_t total_len)
{
    //

    struct rte_ether_hdr *eth = (struct rte_ether_hdr*)msg;
    rte_memcpy(eth->s_addr.addr_bytes, srcmac, RTE_ETHER_ADDR_LEN);
	rte_memcpy(eth->d_addr.addr_bytes, dstmac, RTE_ETHER_ADDR_LEN);
    eth->ether_type = htons(RTE_ETHER_TYPE_IPV4);
 
	// 2 iphdr 
	struct rte_ipv4_hdr *ip = (struct rte_ipv4_hdr *)(msg + sizeof(struct rte_ether_hdr));
	ip->version_ihl = 0x45;
	ip->type_of_service = 0;
	ip->total_length = htons(total_len - sizeof(struct rte_ether_hdr));
	ip->packet_id = 0;
	ip->fragment_offset = 0;
	ip->time_to_live = 64; // ttl = 64
	ip->next_proto_id = IPPROTO_UDP;
	ip->src_addr = sip;
	ip->dst_addr = dip;
	
	ip->hdr_checksum = 0;
	ip->hdr_checksum = rte_ipv4_cksum(ip);


	// 3 udphdr 
	struct rte_udp_hdr *udp = (struct rte_udp_hdr *)(msg + sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr));
	udp->src_port = sport;
	udp->dst_port = dport;
	uint16_t udplen = total_len - sizeof(struct rte_ether_hdr) - sizeof(struct rte_ipv4_hdr);
	udp->dgram_len = htons(udplen);

	rte_memcpy((uint8_t*)(udp+1), data, udplen);

	udp->dgram_cksum = 0;
	udp->dgram_cksum = rte_ipv4_udptcp_cksum(ip, udp);

	return 0;
}

struct rte_mbuf *send_udp_pack(struct rte_mempool *mbuf_pool, uint32_t sip, uint32_t dip, uint16_t sport, uint16_t dport, uint8_t *srcmac, uint8_t *dstmac, uint8_t *data, uint16_t length)
{
    	// mempool --> mbuf

	const unsigned total_len = length + 42;

	struct rte_mbuf *mbuf = rte_pktmbuf_alloc(mbuf_pool);
	if (!mbuf) {
		rte_exit(EXIT_FAILURE, "rte_pktmbuf_alloc\n");
	}
	mbuf->pkt_len = total_len;
	mbuf->data_len = total_len;

	uint8_t *pktdata = rte_pktmbuf_mtod(mbuf, uint8_t*);

	build_udp_pack(pktdata, sip, dip, sport, dport, srcmac, dstmac, data, total_len);

	return mbuf;
}

int recvUDP(struct rte_mbuf *udpmbuf)
{

    printf("recv UDP package\n");
    struct rte_ipv4_hdr *iphdr = rte_pktmbuf_mtod_offset(udpmbuf, struct rte_ipv4_hdr *, sizeof(struct rte_ether_hdr));
    struct rte_udp_hdr *udphdr = (struct rte_udp_hdr *)(iphdr + 1);


    // 发送点数据包IP信息
	//struct in_addr addr;
	//addr.s_addr = iphdr->src_addr;

    // 不接受目的不是本机的数据包
    struct connectConfig* host= getConnectConfigFromIPPort(iphdr->dst_addr, udphdr->dst_port, iphdr->next_proto_id);
    if (host == NULL) {
        rte_pktmbuf_free(udpmbuf);
        return -3;
	} 

    // 将数据从网络包取到本地
    // 数据基础信息
    struct connectData *data = rte_malloc("connectData",sizeof(struct connectData),0);
    if (data == NULL) {
		rte_pktmbuf_free(udpmbuf);
		return -1;
	}
    
    data->dip = iphdr->dst_addr;
    data->sip = iphdr->src_addr;
    data->sport  = udphdr->src_port;
    data->dport = udphdr->dst_port;
    data->protocol = IPPROTO_UDP;
    data->length = ntohs(udphdr->dgram_len);

    // 实际数据字符信息
    data->data = rte_malloc("data",data->length - sizeof(struct rte_udp_hdr),0);
	if (data->data == NULL) {
		rte_pktmbuf_free(udpmbuf);
		rte_free(data);
		return -2;
	}

    rte_memcpy(data->data,(unsigned char*)(udphdr + 1), data->length - sizeof(struct rte_udp_hdr));

    // 将数据从本地送往套接字缓冲区
    rte_ring_mp_enqueue(host->rcvbuf,data);

    pthread_mutex_lock(&host->mutex);
	pthread_cond_signal(&host->cond);
	pthread_mutex_unlock(&host->mutex);

	rte_pktmbuf_free(udpmbuf);

	return 0;
}

int sendUdpPackage(struct rte_mempool* mbuf_pool)
{
    // add all fd upd socker and send  
    struct connectConfig *head = NULL;

    for  (head = headUDPConfig;  head != NULL; head =  head->next)
    {
        struct connectData *data;
        rte_ring_mc_dequeue(head->sndbuf,(void**)&data);


        uint8_t* dstmac = get_dst_mac(data->dip);
        if(dstmac ==NULL)
        {
            struct rte_mbuf *arpbuf =  send_arp_pack(mbuf_pool, RTE_ARP_OP_REQUEST,g_src_mac,data->sip,data->dip);
        
    
            struct inout_ring *ring = ringInstance();
            rte_ring_mp_enqueue_burst(ring->out, (void **)&arpbuf, 1, NULL);
            rte_ring_mp_enqueue(head->sndbuf, data);
       }
       else
       {
            struct rte_mbuf *udpbuf =  send_udp_pack(mbuf_pool,data->sip,data->dip,data->sport,data->dport,g_src_mac,dstmac,data->data,data->length);
            struct inout_ring *ring = ringInstance();
            rte_ring_mp_enqueue_burst(ring->out, (void **)&udpbuf, 1, NULL);
       }
    }
    return 0;
}



int udpApp(__attribute__((unused)) void *arg)
{
    //printf ("App starting !!!!!!!!!!!!!!!!!!!!!!\n");
    // UDP的管道符
    int connfd = nsocket(AF_INET, SOCK_DGRAM, 0);
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
            
            printf("Recv sucessful !!!!!!!!!!!!!!!!!");
            continue;

        }
        else
        {
            // 接收消息失败
			printf("recv from %s:%d, data:%s\n", inet_ntoa(clientaddr.sin_addr), 
				ntohs(clientaddr.sin_port), buffer);
			nsendto(connfd, buffer, strlen(buffer), 0, 
				(struct sockaddr*)&clientaddr, sizeof(clientaddr));
        }
    }

    // 关闭套接字
    nclose(connfd);
}