
#include "../header/globalConfig.h"

uint32_t g_src_ip = MAKE_IPVE_ADDR(10,6,140,51);


const struct rte_eth_conf ifIndex_conf_default = {
    .rxmode = {.max_rx_pkt_len = RTE_ETHER_MAX_LEN}
};

int g_dpdk_ifIndex = 0;