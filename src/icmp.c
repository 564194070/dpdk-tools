#include "../header/protocol/icmp.h"

extern uint32_t g_src_ip;
extern uint8_t g_src_mac[RTE_ETHER_ADDR_LEN];

uint16_t ng_checksum(uint16_t *addr, int count) {

	register long sum = 0;

	while (count > 1) {

		sum += *(unsigned short*)addr++;
		count -= 2;
	
	}

	if (count > 0) {
		sum += *(unsigned char *)addr;
	}

	while (sum >> 16) {
		sum = (sum & 0xffff) + (sum >> 16);
	}

	return ~sum;
}

int build_icmp_packet(uint8_t *msg,uint8_t *dst_mac, uint32_t sip, uint32_t dip, uint16_t id, uint16_t seqnb, uint8_t* data, uint8_t data_len) {

	// 1 ether
	struct rte_ether_hdr *eth = (struct rte_ether_hdr *)msg;
	rte_memcpy(eth->s_addr.addr_bytes, g_src_mac, RTE_ETHER_ADDR_LEN);
	rte_memcpy(eth->d_addr.addr_bytes, dst_mac, RTE_ETHER_ADDR_LEN);
	eth->ether_type = htons(RTE_ETHER_TYPE_IPV4);

	// 2 ip
	struct rte_ipv4_hdr *ip = (struct rte_ipv4_hdr *)(msg + sizeof(struct rte_ether_hdr));
	ip->version_ihl = 0x45;
	ip->type_of_service = 0;
	ip->total_length = htons(sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_icmp_hdr)+ data_len);
	ip->packet_id = 0;
	ip->fragment_offset = 0;
	ip->time_to_live = 64; // ttl = 64
	ip->next_proto_id = IPPROTO_ICMP;
	ip->src_addr = sip;
	ip->dst_addr = dip;
	
	ip->hdr_checksum = 0;
	ip->hdr_checksum = rte_ipv4_cksum(ip);

	// 3 icmp 
	struct rte_icmp_hdr *icmp = (struct rte_icmp_hdr *)(msg + sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr));
	// 类型 
    icmp->icmp_type = RTE_IP_ICMP_ECHO_REPLY;

    // ICMP差错报文的类型
	icmp->icmp_code = 0;
    // 标识符
	icmp->icmp_ident = id;
    // 序列号
	icmp->icmp_seq_nb = seqnb;

    rte_memcpy((uint8_t*)(icmp + 1), data, data_len);

	icmp->icmp_cksum = 0;
	icmp->icmp_cksum = ng_checksum((uint16_t*)icmp, sizeof(struct rte_icmp_hdr)+data_len);

	return 0;
}


struct rte_mbuf *send_icmp_pack(struct rte_mempool *mbuf_pool, uint8_t *dst_mac, uint32_t sip, uint32_t dip, uint16_t id, uint16_t seqnb, uint8_t* data, uint16_t data_len) {

	const unsigned total_length = sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_icmp_hdr) + data_len;

	struct rte_mbuf *mbuf = rte_pktmbuf_alloc(mbuf_pool);
	if (!mbuf) {
		rte_exit(EXIT_FAILURE, "rte_pktmbuf_alloc\n");
	}
	
	mbuf->pkt_len = total_length;
	mbuf->data_len = total_length;

	uint8_t *pkt_data = rte_pktmbuf_mtod(mbuf, uint8_t *); 
	build_icmp_packet(pkt_data, dst_mac, sip, dip, id, seqnb, data, data_len);

	return mbuf;
}