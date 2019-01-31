#include "util.h"
#include "iptables_command.h"
#include <errno.h>
#include <string.h>

void error(uint8_t *msg, int err)
{
	if (err != 0) fprintf(stderr, "[ERROR] %s (%s)\n", msg, strerror(err));
	else fprintf(stderr, "[ERROR] %s\n", msg);
}

bool dying = false;
void die(uint8_t *msg, int err)
{
	if (err != 0) fprintf(stderr, "[FATAL] %s (%s)\n", msg, strerror(err));
	else fprintf(stderr, "[FATAL] %s\n", msg);
	if (!dying)
	{
		dying = true;
		run_iptables_commands(true);
	}
	exit(EXIT_FAILURE);
}

int get_ifindex(uint8_t *dev)
{
	struct ifreq ifr = {0};
	strncpy(ifr.ifr_name, dev, IFNAMSIZ);
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	int r = ioctl(sockfd, SIOGIFINDEX, &ifr);
	if (r < 0) 
	{
		puts(dev);
		die("Error getting ifindex.", errno);
	}
	close(sockfd);
	return ifr.ifr_ifindex;
}

void print_hex(uint8_t *data, int len)
{
	printf("\n****************\n");
	for(int i = 0; i < len; ++i) printf("%02x ", data[i]);
	printf("\n****************\n");
}