#ifndef _COMMON_H
#define _COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>

extern bool ipv6_support;
extern bool enable_ipv6_forwarder;
extern bool enable_ipv4_forwarder;
extern bool set_iptables_nat;
extern bool set_ip6tables_nat;
extern bool set_ip6tables_drop_rst;
extern bool set_iptables_drop_rst;

extern uint8_t lkl_mac_addr[6];
extern uint8_t tap_mac_addr[6];

extern struct in6_addr lkl_ipv6_addr;
extern struct in6_addr tap_ipv6_addr;
extern struct in6_addr listen_ipv6_addr;

extern in_addr_t lkl_ipv4_addr;
extern in_addr_t tap_ipv4_addr;
extern in_addr_t target_ipv4_addr;
extern in_addr_t listen_ipv4_addr;

extern int ipv6_subnet_prefix_len;
extern int ipv4_subnet_prefix_len;

extern uint16_t listen_port;
extern uint16_t target_port;

extern uint8_t lkl_mac_addr[6];
extern uint8_t tap_mac_addr[6];

extern char host_if_name_v6[10];
extern char host_if_name_v4[10];

extern int tap_ifindex;

extern int lkl_mem;

#endif