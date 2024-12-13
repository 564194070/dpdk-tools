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
		}

	}

	
	return NULL;
	
}