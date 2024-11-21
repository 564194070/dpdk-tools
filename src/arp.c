#include "../header/protocol/arp.h"


// 单例模式ARP
static struct arp_table *arpt = NULL;


// arp reply信息
static uint8_t g_default_arp_mac[RTE_ETHER_ADDR_LEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
//static uint8_t g_default_eth_mac[RTE_ETHER_ADDR_LEN] = {0x00};

// 获取目标MAC地址
uint8_t* get_dst_mac(uint32_t dip)
{
    // ARP数据迭代器
    struct arp_entry *iter;
    // ARP数据表表头
    struct arp_table *table = arp_table_instance();

    // 遍历本地ARP解析表
    for (iter = table->entries; iter != NULL; iter = iter->next)
    {
        if (dip == iter->ip)
        {
            return iter->hwaddr;
        }
    }
    return NULL;
}


// 构建arp数据包
int build_arp_packet(uint8_t *msg,uint16_t opcode ,uint8_t* dst_mac, uint32_t src_ip, uint32_t dst_ip)
{
    // 以太网头
    struct rte_ether_hdr *ethhdr = (struct rte_ether_hdr*)msg;
    // 源IP地址
    rte_memcpy(ethhdr->s_addr.addr_bytes, g_src_mac, RTE_ETHER_ADDR_LEN);

    int  i =  0;

	if (0 == strncmp((const char *)dst_mac, (const char *)g_default_arp_mac, RTE_ETHER_ADDR_LEN)) 
    {
		rte_memcpy(ethhdr->d_addr.addr_bytes, dst_mac, RTE_ETHER_ADDR_LEN);
        i  = 1;
	} else {
        
		rte_memcpy(ethhdr->d_addr.addr_bytes, dst_mac, RTE_ETHER_ADDR_LEN);
	}

    ethhdr->ether_type = htons(RTE_ETHER_TYPE_ARP);

    // ARP包
    struct rte_arp_hdr *arp = (struct rte_arp_hdr*)(ethhdr + 1);
    // 硬件 字节序转换
    arp->arp_hardware = htons(1);
    // 协议类型
    arp->arp_protocol = htons(RTE_ETHER_TYPE_IPV4);
    // 硬件地址长度
    arp->arp_hlen = RTE_ETHER_ADDR_LEN;
    // 软件地址长度
    arp->arp_plen = sizeof(uint32_t);
    // 软件操作长度 (请求1和返回2)
    arp->arp_opcode = htons(opcode);

    // 源IP地址
    rte_memcpy(arp->arp_data.arp_sha.addr_bytes, g_src_mac,RTE_ETHER_ADDR_LEN);
    // 目的IP地址
    rte_memcpy(arp->arp_data.arp_tha.addr_bytes, dst_mac,RTE_ETHER_ADDR_LEN);
    if  (i == 1) 
    {
        uint8_t mac[RTE_ETHER_ADDR_LEN] = {0x0};
        rte_memcpy(arp->arp_data.arp_tha.addr_bytes, mac,RTE_ETHER_ADDR_LEN);
    }

    arp->arp_data.arp_sip = src_ip;
    arp->arp_data.arp_tip = dst_ip;
    return 0;
}


// ARP本身的功能，接受和发送
// ARP发起ARP查询,ARP发送ARP响应
struct rte_mbuf* send_arp_pack(struct rte_mempool* mbuf_pool,uint16_t opcode , uint8_t* dst_mac, uint32_t src_ip, uint32_t dst_ip)
{
    // 14字节以太网
    // 28字节ARP

    const int total_len = sizeof(struct rte_ether_hdr) + sizeof(struct rte_arp_hdr);
    // 从内存池分配一个buf存储ARP数据包
    struct rte_mbuf* mbuf = rte_pktmbuf_alloc(mbuf_pool);
    if (!mbuf)
    {
        rte_exit(EXIT_FAILURE, "ARP Create Packet Error");
    }
    mbuf->pkt_len = total_len;
    mbuf->data_len = total_len;
    uint8_t* pkt_data = rte_pktmbuf_mtod(mbuf, uint8_t*);
    build_arp_packet(pkt_data,opcode,dst_mac,src_ip,dst_ip);
    return mbuf;
}



// 定时器
// 定时器回调 参数定时器和需要回调的参数
void arp_request_timer_cb (__attribute__((unused)) struct rte_timer* tim, __attribute__((unused)) void* arg)
{
    printf("触发回调！\n");
    struct rte_mempool* mbuf_pool = (struct rte_mempool*)arg;
    struct inout_ring* ring = ringInstance();

    for (int index = 1; index < 254; ++index)
    {
        struct rte_mbuf* arpbuf = NULL ;


        uint32_t dstip = (g_src_ip & 0x00FFFFFF) | (0xFF000000 & (index << 24));

        uint8_t* dstmac =  get_dst_mac(dstip);
        if (dstmac == NULL)
        {
            struct in_addr addr;
            addr.s_addr = dstip;
            printf(" --> arp send: %s\n", inet_ntoa(addr));
            addr.s_addr = g_src_ip;
            printf(" --> arp src send: %s\n", inet_ntoa(addr));
            //ARP表找不到内容，就发这样
            //arphdr -> mac :  FF:FF:FF:FF:FF:FF
            //ether -> mac:    00:00:00:00:00:00
            //g_default_eth_mac  g_default_arp_mac
            arpbuf = send_arp_pack(mbuf_pool, RTE_ARP_OP_REQUEST,g_default_arp_mac ,g_src_ip,dstip);
        }
        else
        {
            // 能找到ARP信息
            arpbuf = send_arp_pack(mbuf_pool, RTE_ARP_OP_REQUEST, dstmac ,g_src_ip,dstip);
        }
        /*
        rte_eth_tx_burst(g_dpdk_ifIndex,0,&arpbuf,1);
        rte_pktmbuf_free(arpbuf);
        */
       rte_ring_mp_enqueue_burst(ring->out, (void**)&arpbuf, 1, NULL);
    }
}


struct arp_table *arp_table_instance (void)
{
    if (arpt == NULL)
    {
        // name size 对齐
        arpt = (struct arp_table*)rte_malloc("arp_table", sizeof(struct arp_table), 0);
        if (arpt == NULL)
        {
            rte_exit(EXIT_FAILURE, "ARPTable Create Memory Error");
        }
        memset(arpt, 0, sizeof(struct arp_table));
    }

    return arpt;
}