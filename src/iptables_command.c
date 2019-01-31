#include "iptables_command.h"
#include "util.h"
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>

bool ipv6_support;
bool enable_ipv6_forwarder;
bool enable_ipv4_forwarder;

bool set_iptables_nat;
bool set_ip6tables_nat;
bool set_ip6tables_drop_rst;
bool set_iptables_drop_rst;

struct in6_addr lkl_ipv6_addr;
struct in6_addr listen_ipv6_addr;

in_addr_t lkl_ipv4_addr;
in_addr_t listen_ipv4_addr;

uint16_t listen_port;

const char *iptables_cmd = "iptables";
const char *ip6tables_cmd = "ip6tables";
const char *prerouting_cmd_fmt = "%s -t nat -%c PREROUTING -p tcp -d %s --dport %u -j DNAT --to-destination %s:%d";
const char *postrouting_cmd_fmt = "%s -t nat -%c POSTROUTING -p tcp -s %s -j SNAT --to-source %s";
const char *forward_cmd_fmt = "%s -%c FORWARD -m conntrack --ctstate ESTABLISHED,DNAT -j ACCEPT";
const char *drop_rst_cmd_fmt = "%s -%c OUTPUT -s %s -p tcp --sport %u --tcp-flags RST RST -j DROP";

char listen_ipv6_str[INET6_ADDRSTRLEN];
char lkl_ipv6_str[INET6_ADDRSTRLEN];
char listen_ipv4_str[INET_ADDRSTRLEN];
char lkl_ipv4_str[INET_ADDRSTRLEN];

bool rule_is_set[8] = {0};

void init_ip_strings()
{
	inet_ntop(AF_INET, &lkl_ipv4_addr, lkl_ipv4_str, INET_ADDRSTRLEN);
	if (errno) die("error converting lkl_ipv4_addr to string", errno);
	
	inet_ntop(AF_INET, &listen_ipv4_addr, listen_ipv4_str, INET_ADDRSTRLEN);
	if (errno) die("error converting listen_ipv4_str to string", errno);
	
	inet_ntop(AF_INET6, &lkl_ipv6_addr, lkl_ipv6_str, INET6_ADDRSTRLEN);
	if (errno) die("error converting lkl_ipv6_addr to string", errno);
	
	inet_ntop(AF_INET6, &listen_ipv6_addr, listen_ipv6_str, INET6_ADDRSTRLEN);
	if (errno) die("error converting listen_ipv6_addr to string", errno);
}

void run_command(char *cmd)
{
	puts(cmd);
	int ret = system(cmd);
	if (ret != 0)
	{
		printf("Got return value: %d\n", ret);
		die("Command failed.", false);
	}
}


void run_iptables_commands(bool unset)
{
	if (!unset) init_ip_strings();

	char *prerouting_cmd = malloc(strlen(iptables_cmd) + strlen(prerouting_cmd_fmt) + INET_ADDRSTRLEN * 2 + 10);
	char *postrouting_cmd = malloc(strlen(iptables_cmd) + strlen(postrouting_cmd_fmt) + INET_ADDRSTRLEN * 2);
	char *forward_cmd = malloc(strlen(iptables_cmd) + strlen(forward_cmd_fmt));
	
	char *prerouting_cmd_v6 = malloc(strlen(ip6tables_cmd) + strlen(prerouting_cmd_fmt) + INET6_ADDRSTRLEN * 2 + 10);
	char *postrouting_cmd_v6 = malloc(strlen(ip6tables_cmd) + strlen(postrouting_cmd_fmt) + INET6_ADDRSTRLEN * 2);
	char *forward_cmd_v6 = malloc(strlen(ip6tables_cmd) + strlen(forward_cmd_fmt));
	
	char *drop_rst_cmd = malloc(strlen(iptables_cmd) + strlen(drop_rst_cmd_fmt) + INET_ADDRSTRLEN + 5);
	char *v6_drop_rst_cmd = malloc(strlen(ip6tables_cmd) + strlen(drop_rst_cmd_fmt) + INET6_ADDRSTRLEN + 5);
	
	char op = unset ? 'D' : 'A';

	char lkl_ipv6_str_escaped[INET6_ADDRSTRLEN + 2];
	sprintf(lkl_ipv6_str_escaped, "[%s]", lkl_ipv6_str);
	
	sprintf(prerouting_cmd, prerouting_cmd_fmt, iptables_cmd, op, listen_ipv4_str, listen_port, lkl_ipv4_str, listen_port);
	sprintf(postrouting_cmd, postrouting_cmd_fmt, iptables_cmd, op, lkl_ipv4_str, listen_ipv4_str);
	sprintf(forward_cmd, forward_cmd_fmt, iptables_cmd, op);
	
	sprintf(prerouting_cmd_v6, prerouting_cmd_fmt, ip6tables_cmd, op, listen_ipv6_str, listen_port, lkl_ipv6_str_escaped, listen_port);
	sprintf(postrouting_cmd_v6, postrouting_cmd_fmt, ip6tables_cmd, op, lkl_ipv6_str, listen_ipv6_str);
	sprintf(forward_cmd_v6, forward_cmd_fmt, iptables_cmd, op);
	
	sprintf(drop_rst_cmd, drop_rst_cmd_fmt, iptables_cmd, op, listen_ipv4_str, listen_port);
	sprintf(v6_drop_rst_cmd, drop_rst_cmd_fmt, ip6tables_cmd, op, listen_ipv6_str, listen_port);

	
	if (set_iptables_nat)
	{
		if (!unset)
		{
			int ret = system("bash -c 'exit $[ $(cat /proc/sys/net/ipv4/ip_forward) != 1 ]'");
			if (ret != 0) die("You should set net.ipv4.ip_forward to 1.", false);
		}
		
		if (!unset || rule_is_set[0]) run_command(prerouting_cmd);
		if (!unset) rule_is_set[0] = true;
		
		if (!unset || rule_is_set[1]) run_command(postrouting_cmd);
		if (!unset) rule_is_set[1] = true;
		
		if (!unset || rule_is_set[2]) run_command(forward_cmd);
		if (!unset) rule_is_set[2] = true;
	}
	else if (enable_ipv4_forwarder && set_iptables_drop_rst)
	{
		if (!unset || rule_is_set[3]) run_command(drop_rst_cmd);
		if (!unset) rule_is_set[3] = true;
	}
	
	if (set_ip6tables_nat)
	{
		if (!unset)
		{
			int ret = system("bash -c 'exit $[ $(cat /proc/sys/net/ipv6/conf/all/forwarding) != 1 ]'");
			if (ret != 0) die("You should set net.ipv6.conf.all.forwarding to 1.", false);
		}
		
		if (!unset || rule_is_set[4]) run_command(prerouting_cmd_v6);
		if (!unset) rule_is_set[4] = true;
		
		if (!unset || rule_is_set[5]) run_command(postrouting_cmd_v6);
		if (!unset) rule_is_set[5] = true;
		
		if (!unset || rule_is_set[6]) run_command(forward_cmd_v6);
		if (!unset) rule_is_set[6] = true;
	}
	else if (enable_ipv6_forwarder && set_ip6tables_drop_rst)
	{
		if (!unset || rule_is_set[7]) run_command(v6_drop_rst_cmd);
		if (!unset) rule_is_set[7] = true;
	}
	
	free(prerouting_cmd);
	free(postrouting_cmd);
	free(forward_cmd);
	free(prerouting_cmd_v6);
	free(postrouting_cmd_v6);
	free(forward_cmd_v6);
	free(drop_rst_cmd);
	free(v6_drop_rst_cmd);
	
}