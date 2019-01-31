#include "tap.h"
#include <errno.h>

int tap_ifindex;
uint8_t tap_mac_addr[6];
in_addr_t tap_ipv4_addr;
struct in6_addr tap_ipv6_addr;
int ipv6_subnet_prefix_len;
int ipv4_subnet_prefix_len;
bool ipv6_support;

struct sockaddr_in prefix_len_to_saddrin(int len)
{
	uint32_t addr = 0;
	for (int i = 0; i < len; ++i)
	{
		addr |= 1 << i;
	}
	struct sockaddr_in ret = {0};
	ret.sin_family = AF_INET;
	ret.sin_addr.s_addr = addr;
	return ret;
}

int tap_alloc()
{
	struct in6_ifreq {
    	struct in6_addr addr;
		uint32_t prefixlen;
		unsigned int ifindex;
	} in6_ifr;

	struct sockaddr_in saddr;
	struct ifreq ifr;
	int fd, err, sockfd, sock6fd;

	saddr.sin_addr.s_addr = tap_ipv4_addr;
	saddr.sin_family = AF_INET;

	memcpy(&in6_ifr.addr, &tap_ipv6_addr, sizeof(in6_ifr.addr));
	
	in6_ifr.prefixlen = ipv6_subnet_prefix_len;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	sock6fd = socket(AF_INET6, SOCK_DGRAM, 0);

	fd = open("/dev/net/tun", O_RDWR);

	memset(&ifr, 0, sizeof(ifr));

	ifr.ifr_flags = IFF_TAP | IFF_NO_PI;

	strncpy(ifr.ifr_name, (uint8_t *) "tap%d", IFNAMSIZ);
	if((err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0)
	{
		close(fd);
		die("Error creating tap device: %d. \n", errno);
	}

	ioctl(sockfd, SIOGIFINDEX, &ifr);
	in6_ifr.ifindex = ifr.ifr_ifindex;
	tap_ifindex = ifr.ifr_ifindex;

	memcpy(&ifr.ifr_hwaddr.sa_data, &tap_mac_addr, 6);
	ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER; 
	err = ioctl(sockfd, SIOCSIFHWADDR, (void *) &ifr);
	if (err < 0) die("Error assigning mac address to tap interface.", errno);
	
	ifr.ifr_addr = *(struct sockaddr *) &saddr;
	err = ioctl(sockfd, SIOCSIFADDR, (void *) &ifr);
	if (err < 0) die("Error setting ipv4 address to tap interface.", errno);
	
	struct sockaddr_in v4_mask = prefix_len_to_saddrin(ipv4_subnet_prefix_len);
	ifr.ifr_netmask = *(struct sockaddr *) &v4_mask;
	err = ioctl(sockfd, SIOCSIFNETMASK, (void *) &ifr);
	if (err < 0) die("Error setting ipv4 netmask to tap interface.", errno);

	if (ipv6_support)
	{
		err = ioctl(sock6fd, SIOCSIFADDR, (void *) &in6_ifr);
		if (err < 0) die("Error setting ipv6 address to tap interface.", errno);
	}
	
	ifr.ifr_flags = IFF_UP | IFF_PROMISC;
	err = ioctl(sockfd, SIOCSIFFLAGS, (void *) &ifr);
	if (err < 0) die("Error bringing up tap interface.", errno);
	
	printf("%s created.\n", ifr.ifr_name);

	int fd_flags = fcntl(fd, F_GETFD, NULL);
	fcntl(fd, F_SETFL, fd_flags | O_NONBLOCK);

	close(sockfd);
	close(sock6fd);

	return fd;
}