#include <rte_ethdev.h>

// 计算IP的宏定义
#define MAKE_IPVE_ADDR(a,b,c,d)(a + (b<<8) + (c<<16) + (d<<24))


// 打印MAC地址
void print_mac(const char* what, const struct rte_ether_addr* eth_addr);

