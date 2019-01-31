#ifndef _IPV6_FORWARDING_H
#define _IPV6_FORWARDING_H

#include "common.h"
#include "util.h"

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>

#define BUFSIZE 4096

void start_ip_forwarder(bool ipv6);

#endif