#ifndef _UTIL_H
#define _UTIL_H

#include "common.h"
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>

extern bool ipv6_support;
extern uint8_t lkl_mac_addr[6];
extern uint8_t tap_mac_addr[6];

extern int tap_ifindex;

void error(uint8_t *msg, int err);
void die(uint8_t *msg, int err);
int get_ifindex(uint8_t *dev);
void print_hex(uint8_t *data, int len);

#endif