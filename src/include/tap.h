#ifndef _TAP_H
#define _TAP_H

#include "common.h"
#include "util.h"

#include <linux/if_tun.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if_arp.h>
#include <arpa/inet.h>

int tap_alloc();

#endif