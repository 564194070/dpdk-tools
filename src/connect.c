#include  "../header/connect.h"


struct connectConfig *headUDPConfig = NULL;

struct connectConfig *getConnectConfigFromFd(int fd)
{
    struct connectConfig *host;

    for (host = headUDPConfig; host != NULL; host = host->next)
    {
        if (host->fd == fd)
        {
            return host;
        }
    }
    return NULL;
}



struct connectConfig * getConnectConfigFromIPPort(uint32_t dip, uint16_t port, uint8_t proto) {

	struct connectConfig *host;

	for (host = headUDPConfig; host != NULL;host = host->next) {

		if (dip == host->localip && port == host->localport && proto == host->protocol) {
			return host;
		} else {
            if (dip != host->localip)
            {
                printf("ip error -------------------\n");
            }
            else if (port != host->localport)
            {
                printf("port error -------------------\n");
            }
            else if (proto == host->protocol)
            {
                printf("proto error -------------------\n");
            }
        }

	}

	
	return NULL;
	
}