#include <signal.h>
#include <lkl.h>
#include <lkl_host.h>
#include <string.h>

#include "common.h"
#include "config_parser.h"
#include "ip_forwarder.h"
#include "tap.h"
#include "tcp_forwarding.h"
#include "iptables_command.h"

int lkl_mem;
in_addr_t lkl_ipv4_addr;
in_addr_t tap_ipv4_addr;
struct in6_addr lkl_ipv6_addr;
struct in6_addr tap_ipv6_addr;
int ipv6_subnet_prefix_len;
int ipv4_subnet_prefix_len;
bool ipv6_support;
bool enable_ipv6_forwarder;
bool enable_ipv4_forwarder;

void sigterm_handler(int signum)
{
	puts("Terminating...");
	run_iptables_commands(true);
	puts("Bye.");
	exit(0);
}

void print_usage(char *cmd)
{
	printf("Usage: %s -c <cfg_path>\n", cmd);
	exit(1);
}

int main(int argc, char **argv)
{
	char *cfg_path = NULL;
	for(int i = 1; i < argc; ++i)
	{
		if(strcmp(argv[i], "-c") == 0 && i < argc - 1)
		{
			cfg_path = argv[i + 1];
			++i;
		}
		else
		{
			print_usage(argv[0]);
		}
	}
	
	if (cfg_path == NULL) print_usage(argv[0]);
	
	read_config(cfg_path);
	run_iptables_commands(false);

	signal(SIGPIPE, SIG_IGN);
	signal(SIGHUP, sigterm_handler);
	signal(SIGTERM, sigterm_handler);
	signal(SIGINT, sigterm_handler);

	char lkl_param[50] = {0};
	sprintf(lkl_param, "mem=%dM loglevel=8", lkl_mem);
	lkl_start_kernel(&lkl_host_ops, lkl_param);

	lkl_sysctl("net.core.default_qdisc", "fq");
	lkl_sysctl("net.ipv4.tcp_congestion_control", "bbr");
	lkl_sysctl("net.ipv4.tcp_wmem", "4096 87380 21474830");

	int tap_fd = tap_alloc();
	
	struct lkl_netdev *nd = (struct lkl_netdev *) lkl_register_netdev_fd(tap_fd, tap_fd);
	
	struct lkl_netdev_args netdev_args = {
		.mac = lkl_mac_addr,
		.offload= 0,
	};

	int nd_id = lkl_netdev_add(nd, &netdev_args);

	int nd_ifindex = lkl_netdev_get_ifindex(nd_id);

	lkl_if_up(1);
	lkl_if_up(nd_ifindex);

	int b = lkl_if_set_ipv4(nd_ifindex, lkl_ipv4_addr, ipv4_subnet_prefix_len);
	lkl_set_ipv4_gateway(tap_ipv4_addr);
	
	lkl_if_set_ipv6(nd_ifindex, &lkl_ipv6_addr, ipv6_subnet_prefix_len);
	lkl_set_ipv6_gateway(&tap_ipv6_addr);
	
	if (enable_ipv6_forwarder && ipv6_support) start_ip_forwarder(true);
	if (enable_ipv4_forwarder) start_ip_forwarder(false);
	tcp_forward_server();

	return 0;
}
