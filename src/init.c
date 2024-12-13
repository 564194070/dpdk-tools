#include "../header/init.h"

extern uint8_t g_src_mac[RTE_ETHER_ADDR_LEN];

// 初始化网卡
void ifIndex_init (struct rte_mempool * mbuf_pool)
{
    // 检查可用网卡数量
    // 1.检测端口是否合法
    // 绑定了多少个网卡绑定了PCIE IGB VFIO啥的
    // 获取默认网卡信息，还没有添加DPDK信息
    uint16_t ifIndex_ports = rte_eth_dev_count_avail();
    if (ifIndex_ports == 0)
    {
        rte_exit(EXIT_FAILURE, "Select Eth Not Ready\n");
    }

    // 获取网卡基本信息
    struct rte_eth_dev_info dev_info;
    rte_eth_dev_info_get(g_dpdk_ifIndex, &dev_info);

    // 2.添加DPDK基本信息
    // 设置发送接收队列
    const int num_rx_queues = 1;
    const int num_tx_queues = 1;
    struct rte_eth_conf ifIndex_config = ifIndex_conf_default;
    rte_eth_dev_configure(g_dpdk_ifIndex, num_rx_queues, num_tx_queues, &ifIndex_config);

    // 设置网卡
    int ret;
    // 配置接收队列
    ret = rte_eth_rx_queue_setup(g_dpdk_ifIndex, 0, 128, rte_eth_dev_socket_id(g_dpdk_ifIndex), NULL, mbuf_pool);
    if (ret < 0)
    {
        rte_exit(EXIT_FAILURE, "Setup Rx Queue Error\n");
    }
    // 配置发送队列
    ret = -1;
    struct rte_eth_txconf txq_conf = dev_info.default_txconf;
    // tx队列的负载，接受和发送同步
    txq_conf.offloads = ifIndex_config.rxmode.offloads;
    // 网口，队列，队列最大包负载，socketid，配置信息
    // 512 < n < 4096
    ret = rte_eth_tx_queue_setup(g_dpdk_ifIndex, 0, 1024, rte_eth_dev_socket_id(g_dpdk_ifIndex),&txq_conf);
    if (ret < 0)
    {
        rte_exit(EXIT_FAILURE, "Setup Tx Queue Error\n");
    }

    // 启动网卡
    ret = -1;
    ret = rte_eth_dev_start(g_dpdk_ifIndex);
    if (ret < 0)
    {
        rte_exit(EXIT_FAILURE, "Start Eth Error\n");
    }

    printf("初始化DPDK成功\n");
    // 读取MAC地址
    //struct ether_addr addr_mac;
    //rte_eth_macaddr_get(g_dpdk_ifIndex, &addr_mac);

    //开启网卡混杂模式
    //rte_eth_promiscuous_get(g_dpdk_ifIndex);

}  


// 初始化定时器
void timer_init(struct rte_mempool * mbuf_pool) 
{
  
    // 初始化定时器
    rte_timer_subsystem_init();
    static struct rte_timer arp_timer;
    rte_timer_init(&arp_timer);

    // 设置定时器频率
    uint64_t  hz = rte_get_timer_hz();
    unsigned lcore_id = rte_lcore_id();

    // 设置定时器回调
    rte_timer_reset(&arp_timer,hz,PERIODICAL,lcore_id,arp_request_timer_cb,mbuf_pool);
}

// 初始化环
void ring_init(void)   
{
    struct inout_ring *ring = ringInstance();
	if (ring == NULL) {
		rte_exit(EXIT_FAILURE, "ring buffer init failed\n");
	}

	if (ring->in == NULL) {
		ring->in = rte_ring_create("in ring", RING_SIZE, rte_socket_id(), RING_F_SP_ENQ | RING_F_SC_DEQ);
	}
	if (ring->out == NULL) {
		ring->out = rte_ring_create("out ring", RING_SIZE, rte_socket_id(), RING_F_SP_ENQ | RING_F_SC_DEQ);
	}
}


int initDPDK  (int argc, char* argv[])
{
    //  启动基础运行环境
    int res = -1;
    res = rte_eal_init(argc, argv);
    if (res < 0)
    {
        rte_exit(EXIT_FAILURE, "Init DPDK Failed\n");
    }

    //  初始化内存池
    struct rte_mempool* mbuf_pool = rte_pktmbuf_pool_create("mbuf_pool", NUMMBUFS,0,0,RTE_MBUF_DEFAULT_BUF_SIZE,rte_socket_id());
    if (mbuf_pool == NULL)
    {
        rte_exit(EXIT_FAILURE, "Could Not Create Buffer\n");
    }

    // 初始化网卡,启动DPDK
    ifIndex_init(mbuf_pool);
    rte_eth_macaddr_get(g_dpdk_ifIndex,(struct rte_ether_addr* )g_src_mac);

    unsigned lcore_id = rte_lcore_id();
    // 初始化定时器
    timer_init(mbuf_pool);

    // 初始化环
    ring_init();

    // 初始化多核运行环境
    printf("初始化工作线程成功\n");
    lcore_id = rte_get_next_lcore(lcore_id, 1, 0);
    rte_eal_remote_launch(pkt_process, mbuf_pool, lcore_id);
    lcore_id = rte_get_next_lcore(lcore_id, 1, 0);
    rte_eal_remote_launch(udpApp, mbuf_pool, lcore_id);

    // 自己这个0号核心就干一件事，接收数据包，放入环中。从环中拿数组包，发送。
    struct inout_ring *ring = ringInstance();
    while (1) 
    {
		// rx
		struct rte_mbuf *rx[BURSTSIZE];
		unsigned num_recvd = rte_eth_rx_burst(g_dpdk_ifIndex, 0, rx, BURSTSIZE);
		if (num_recvd > BURSTSIZE) {
			rte_exit(EXIT_FAILURE, "Error receiving from eth\n");
		} else if (num_recvd > 0) {
            // 将数据报文送入环中
			rte_ring_sp_enqueue_burst(ring->in, (void**)rx, num_recvd, NULL);
		}

		
		// tx
		struct rte_mbuf *tx[BURSTSIZE];
		unsigned nb_tx = rte_ring_sc_dequeue_burst(ring->out, (void**)tx, BURSTSIZE, NULL);
		if (nb_tx > 0) {

			rte_eth_tx_burst(g_dpdk_ifIndex, 0, tx, nb_tx);

			unsigned i = 0;
			for (i = 0;i < nb_tx;i ++) {
				rte_pktmbuf_free(tx[i]);
			}
			
		}
	
		static uint64_t prev_tsc = 0, cur_tsc;
		uint64_t diff_tsc;


        
		cur_tsc = rte_rdtsc();
		diff_tsc = cur_tsc - prev_tsc;
        //printf("prev:%I64d, cur_tsc:%I64d, diff_tsc:%I64d \n",prev_tsc,cur_tsc,diff_tsc);
		if (diff_tsc > TIMER_RESOLUTION_CYCLES) {
            //printf("触发-----------diff_tsc:%I64d \n",diff_tsc);
			rte_timer_manage();
			prev_tsc = cur_tsc;
		}

	}
}
