#include "../header/work.h"

int pkt_process(void *arg)
{
    // 获取内存池
	struct rte_mempool *mbuf_pool = (struct rte_mempool *)arg;
    // 获取收发缓冲区
	struct inout_ring *ring = ringInstance();



    while (1)
    {
        // 接受数据包的缓冲区
        struct rte_mbuf *mbufs[BURSTSIZE];
        unsigned recv_package = rte_ring_mc_dequeue_burst(ring->in, (void **)mbufs, BURSTSIZE, NULL);

        // 遍历处理每个数据包
        for (unsigned index = 0; index < recv_package; ++index)
        {
            //分析一层报头
            //printf("接收到数据包\n");
            struct rte_ether_hdr *eth_hdr = rte_pktmbuf_mtod(mbufs[index], struct rte_ether_hdr*);

            // 辨别二层协议
            if (eth_hdr->ether_type == rte_cpu_to_be_16(RTE_ETHER_TYPE_ARP))
            {
                // 解析ARP报文
                struct rte_arp_hdr* arp_hdr = rte_pktmbuf_mtod_offset(mbufs[index], struct rte_arp_hdr*,sizeof(struct rte_ether_hdr));

                /*
                struct in_addr addr;
                addr.s_addr = arp_hdr->arp_data.arp_tip;
                struct in_addr addr2;
                addr2.s_addr = arp_hdr->arp_data.arp_sip;
                */
                
                //printf("接收到arp响应 ---> src: %s dst %s ", inet_ntoa(addr),inet_ntoa(addr2));
                // 比对IP，处理自身相关数据包
                if (arp_hdr->arp_data.arp_tip == g_src_ip)
                {
                    printf("接受到本机报文\n");
                    //分别处理ARP的发送和接受请求
                    if (arp_hdr->arp_opcode == rte_cpu_to_be_16(RTE_ARP_OP_REQUEST))
                    {
                        // 处理接收到的ARP请求 构建对应的ARP响应
                        struct rte_mbuf* arpbuf = send_arp_pack(mbuf_pool,RTE_ARP_OP_REPLY,arp_hdr->arp_data.arp_sha.addr_bytes,arp_hdr->arp_data.arp_tip,arp_hdr->arp_data.arp_sip);
                        rte_ring_mp_enqueue_burst(ring->out, (void**)&arpbuf, 1, NULL);
                        //rte_pktmbuf_free(mbufs[index]);
                    }
                    else if (arp_hdr->arp_opcode == rte_cpu_to_be_16(RTE_ARP_OP_REPLY))
                    {
                        // 处理ARP响应，添加到ARP缓存
                        printf("接受到ARP响应报文\n");
                        struct arp_table* table = arp_table_instance();
                        // 获取ARP核心的MAC和IP地址
                        uint8_t* arp_hw_addr = get_dst_mac(arp_hdr->arp_data.arp_sip);
                        if (arp_hw_addr == NULL)
                        {
                            struct arp_entry* entry = rte_malloc("arp entry",sizeof(struct arp_entry), 0);
                            if (entry)
                            {
                                memset(entry, 0, sizeof(struct arp_entry));
                                entry->ip = arp_hdr->arp_data.arp_sip;
                                rte_memcpy(entry->hwaddr, arp_hdr->arp_data.arp_sha.addr_bytes, RTE_ETHER_ADDR_LEN);
                                entry->type = ARP_ENTRY_STATUS_DYNAMIC;
                                LL_ADD(entry,table->entries);
                                table->count ++;
                            }
                        }
                        // todo 为啥这块释放了内存
                        rte_pktmbuf_free(mbufs[index]);
                    }
                    continue;

                }
            }
            else if (eth_hdr->ether_type == rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4))
            {
                // 处理IPV4数据包
                struct rte_ipv4_hdr *iphdr =  rte_pktmbuf_mtod_offset(mbufs[index], struct rte_ipv4_hdr *, sizeof(struct rte_ether_hdr));
                // 分别处理TCP/UDP/ICMP数据包
                if (iphdr->next_proto_id == IPPROTO_UDP)
                {
                    //struct rte_udp_hdr *udphdr = (struct rte_udp_hdr *)(iphdr + 1);

                    // 构建UDP回应五元组
                    //rte_memcpy(g_dst_mac, eth_hdr->s_addr.addr_bytes, RTE_ETHER_ADDR_LEN);
				    //rte_memcpy(&g_src_ip, &iphdr->dst_addr, sizeof(uint32_t));
				    //rte_memcpy(&g_dst_ip, &iphdr->src_addr, sizeof(uint32_t));
				    //rte_memcpy(&g_src_port, &udphdr->dst_port, sizeof(uint16_t));
				    //rte_memcpy(&g_dst_port, &udphdr->src_port, sizeof(uint16_t));

                    //uint16_t length = ntohs(udphdr->dgram_len);
                    //*((char*)udphdr + length) = '\0';
                    //struct rte_mbuf *txbuf = send_udp_pack(mbuf_pool,(uint8_t *)(udphdr + 1), length);
                    //rte_ring_mp_enqueue_burst(ring->out,(void**)&txbuf,1,NULL);
                    //rte_pktmbuf_free(mbufs[index]);
                }
                else if (iphdr->next_proto_id == IPPROTO_ICMP)
                {
                    // 处理ICMP报头
                    struct rte_icmp_hdr *icmphdr = (struct rte_icmp_hdr *)(iphdr + 1);
                    printf("接收到icmp报文 --->");
                    uint8_t *data = (uint8_t *)(icmphdr + 1);

                    if (icmphdr->icmp_type == RTE_IP_ICMP_ECHO_REQUEST)
                    {
                        uint16_t data_len = ntohs(iphdr->total_length) - sizeof(struct rte_ipv4_hdr) - sizeof(struct rte_icmp_hdr);
                        printf("数据长度为%d,ip头长度为:%ld,icmp头长度为%ld\n",ntohs(iphdr->total_length), sizeof(struct rte_ipv4_hdr), sizeof(struct rte_icmp_hdr));
                        
                        struct rte_mbuf *txbuf = send_icmp_pack(mbuf_pool, eth_hdr->s_addr.addr_bytes,
						iphdr->dst_addr, iphdr->src_addr, icmphdr->icmp_ident, icmphdr->icmp_seq_nb,data,data_len);
                        
                        rte_ring_mp_enqueue_burst(ring->out, (void**)&txbuf, 1, NULL);
                        rte_pktmbuf_free(mbufs[index]);
                    }
                }
            }
            else
            {
                continue;
            }

        }
    }
    return 0;
}