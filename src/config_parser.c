#include <arpa/inet.h>
#include "config_parser.h"

#include <errno.h>

bool ipv6_support = false;
bool enable_ipv6_forwarder = false;
bool enable_ipv4_forwarder = false;

bool set_iptables_nat = false;
bool set_ip6tables_nat = false;
bool set_ip6tables_drop_rst = false;
bool set_iptables_drop_rst = false;

uint8_t lkl_mac_addr[6];
uint8_t tap_mac_addr[6];

struct in6_addr lkl_ipv6_addr;
struct in6_addr tap_ipv6_addr;
struct in6_addr listen_ipv6_addr;

in_addr_t lkl_ipv4_addr;
in_addr_t tap_ipv4_addr;
in_addr_t target_ipv4_addr;
in_addr_t listen_ipv4_addr;

int ipv6_subnet_prefix_len;
int ipv4_subnet_prefix_len;

uint16_t listen_port;
uint16_t target_port;

int lkl_mem;

uint8_t lkl_mac_addr[6];
uint8_t tap_mac_addr[6];

char host_if_name_v6[10];
char host_if_name_v4[10];

void str_to_mac(char *s, uint8_t *target)
{
	int step = 0;
	if (strlen(s) == 12) step = 2;
	else if (strlen(s) == 17) step = 3;
	else die("Error parsing mac address from config. 1", 0);
	for (int i = 0; i < 6; ++i)
	{
		char octet_str[4] = {0};
		memcpy(octet_str, s + i * step, step);
		unsigned int octet = 0;
		int r = sscanf(octet_str, "%02x", &octet);
		if (r != 1) die("Error parsing mac address from config. 2", 0);
		target[i] = octet;
	}
	if ((target[0] & 0b11) != 0b10)
	{
		print_hex(target, 6);
		printf("Please use a locally administered unicast MAC address. \n"
			"Vaild addresses are: \n"
			"x2-xx-xx-xx-xx-xx, x6-xx-xx-xx-xx-xx, xA-xx-xx-xx-xx-xx, xE-xx-xx-xx-xx-xx\n");
		exit(1);
	}	
}

void read_config(char *path)
{
	printf("Loading config file: '%s'.\n", path);
	FILE *fp = fopen(path, "r");
	if (fp == NULL || errno != 0) die("Error opening config file.", errno);
	ssize_t n_read = 0;
	char *line = NULL;
	size_t n = 0;
	bool failed = false;
	while (n_read = getline(&line, &n, fp) >= 0)
	{
		char *line_stripped = malloc(strlen(line) + 1);
		for(int i = 0; i < strlen(line) + 1; ++i) line_stripped[i] = 0; 
		int cp = 0;
		for(int i = 0; i < strlen(line) + 1; ++i)
		{
			if (!(line[i] == '\t' || line[i] == '\r' || line[i] == '\n' || line[i] == ' '))
			{
				if (line[i] == '#') break;
				line_stripped[cp] = line[i];
				cp++;
			}			
		}
		if (strlen(line_stripped) == 0) continue;
		puts(line_stripped);
		
		char *st = strtok(line_stripped, "=");
		if (st == NULL) die("Error parsing configuration file.", 0);
		//------------------------------
		if (strcmp(st, "ip6_support") == 0)
		{
			st = strtok(NULL, "=");
			if (strcmp(st, "true") == 0) ipv6_support = true;
			else if (strcmp(st, "false") == 0) ipv6_support = false;
			else failed = true;
		}
		else if (strcmp(st, "enable_ip6_forwarder") == 0)
		{
			st = strtok(NULL, "=");
			if (strcmp(st, "true") == 0) enable_ipv6_forwarder = true;
			else if (strcmp(st, "false") == 0) enable_ipv6_forwarder = false;
			else failed = true;
		}
		else if (strcmp(st, "enable_ip_forwarder") == 0)
		{
			st = strtok(NULL, "=");
			if (strcmp(st, "true") == 0) enable_ipv4_forwarder = true;
			else if (strcmp(st, "false") == 0) enable_ipv4_forwarder = false;
			else failed = true;
		}
		else if (strcmp(st, "set_iptables_nat") == 0)
		{
			st = strtok(NULL, "=");
			if (strcmp(st, "true") == 0) set_iptables_nat = true;
			else if (strcmp(st, "false") == 0) set_iptables_nat = false;
			else failed = true;
		}
		else if (strcmp(st, "set_ip6tables_nat") == 0)
		{
			st = strtok(NULL, "=");
			if (strcmp(st, "true") == 0) set_ip6tables_nat = true;
			else if (strcmp(st, "false") == 0) set_ip6tables_nat = false;
			else failed = true;
		}
		else if (strcmp(st, "set_ip6tables_drop_rst") == 0)
		{
			st = strtok(NULL, "=");
			if (strcmp(st, "true") == 0) set_ip6tables_drop_rst = 1;
			else if (strcmp(st, "false") == 0) set_ip6tables_drop_rst = 0;
			else failed = true;
		}
		else if (strcmp(st, "set_iptables_drop_rst") == 0)
		{
			st = strtok(NULL, "=");
			if (strcmp(st, "true") == 0) set_iptables_drop_rst = 1;
			else if (strcmp(st, "false") == 0) set_iptables_drop_rst = 0;
			else failed = true;
		}
		//------------------------------
		else if(strcmp(st, "lkl_mac_address") == 0)
		{
			st = strtok(NULL, "=");
			if (st != NULL) str_to_mac(st, lkl_mac_addr);
		}
		else if(strcmp(st, "tap_mac_address") == 0)
		{
			st = strtok(NULL, "=");
			if (st != NULL) str_to_mac(st, tap_mac_addr);
		}
		//------------------------------
		else if(strcmp(st, "lkl_ip_address") == 0)
		{
			st = strtok(NULL, "=");
			if (st != NULL) 
			{
				lkl_ipv4_addr = inet_addr(st);
				if (lkl_ipv4_addr == -1) failed = true;
			}
		}
		else if(strcmp(st, "tap_ip_address") == 0)
		{
			st = strtok(NULL, "=");
			if (st != NULL)
			{
				tap_ipv4_addr = inet_addr(st);
				if (tap_ipv4_addr == -1) failed = true;
			}
		}
		else if(strcmp(st, "target_ip_address") == 0)
		{
			st = strtok(NULL, "=");
			if (st != NULL)
			{
				target_ipv4_addr = inet_addr(st);
				if (target_ipv4_addr == -1) failed = true;
			}
		}
		else if(strcmp(st, "listen_ip_address") == 0)
		{
			st = strtok(NULL, "=");
			if (st != NULL)
			{
				listen_ipv4_addr = inet_addr(st);
				if (listen_ipv4_addr == -1) failed = true;
			}
		}
		//------------------------------
		else if(strcmp(st, "lkl_ip6_address") == 0)
		{
			st = strtok(NULL, "=");
			if (st != NULL) 
			{
				int r = inet_pton(AF_INET6, st, &lkl_ipv6_addr);
				if (r != 1) failed = true;
			}
		}
		else if(strcmp(st, "tap_ip6_address") == 0)
		{
			st = strtok(NULL, "=");
			if (st != NULL) 
			{
				int r = inet_pton(AF_INET6, st, &tap_ipv6_addr);
				if (r != 1) failed = true;
			}
		}
		else if(strcmp(st, "listen_ip6_address") == 0)
		{
			st = strtok(NULL, "=");
			if (st != NULL) 
			{
				int r = inet_pton(AF_INET6, st, &listen_ipv6_addr);
				if (r != 1) failed = true;
			}
		}
		//------------------------------
		else if(strcmp(st, "lkl_ip_subnet_prefix_len") == 0)
		{
			st = strtok(NULL, "=");
			if (st != NULL)
			{
				int r = sscanf(st, "%d", &ipv4_subnet_prefix_len);
				if (ipv4_subnet_prefix_len < 0 || ipv4_subnet_prefix_len > 32)
				{
					printf("lkl_ip_subnet_prefix_len range error\n");
					failed = true;
				}
			}
		}
		else if(strcmp(st, "lkl_ip6_subnet_prefix_len") == 0)
		{
			st = strtok(NULL, "=");
			if (st != NULL)
			{
				int r = sscanf(st, "%d", &ipv6_subnet_prefix_len);
				if (ipv6_subnet_prefix_len < 0 || ipv6_subnet_prefix_len > 128)
				{
					printf("lkl_ip6_subnet_prefix_len range error\n");
					failed = true;
				}
			}
		}
		else if(strcmp(st, "listen_port") == 0)
		{
			st = strtok(NULL, "=");
			if (st != NULL)
			{
				int r = sscanf(st, "%hu", &listen_port);
				if (listen_port < 1 || listen_port > 65535)
				{
					printf("listen_port range error\n");
					failed = true;
				}
			}
		}
		else if(strcmp(st, "target_port") == 0)
		{
			st = strtok(NULL, "=");
			if (st != NULL)
			{
				int r = sscanf(st, "%hu", &target_port);
				if (target_port < 1 || target_port > 65535)
				{
					printf("target_port range error\n");
					failed = true;
				}
			}
		}
		else if(strcmp(st, "lkl_mem") == 0)
		{
			st = strtok(NULL, "=");
			if (st != NULL)
			{
				int r = sscanf(st, "%dM", &lkl_mem);
				if (lkl_mem < 32 || lkl_mem > 2048)
				{
					printf("lkl_mem range error\n");
					failed = true;
				}
			}
		}
		//------------------------------
		else if(strcmp(st, "ip6_forwarder_listen_interface") == 0)
		{
			st = strtok(NULL, "=");
			if (st != NULL)
			{
				strncpy(host_if_name_v6, st, 9);
			}
		}
		else if(strcmp(st, "ip_forwarder_listen_interface") == 0)
		{
			st = strtok(NULL, "=");
			if (st != NULL)
			{
				strncpy(host_if_name_v4, st, 9);
			}
		}
		//------------------------------
		else
		{
			printf("Unknown option: '%s'.\n", st);
			exit(-1);
		}
		if (st == NULL) die("Error parsing configuration file. 1", 0);
		if (failed)
		{
			printf("Failed to parse: '%s'.\n", st);
			exit(-1);
		}
		st = strtok(NULL, "=");
		if (st != NULL) die("Error parsing configuration file. 2", 0);
		free(line_stripped);
	}
	fclose(fp);
	if (enable_ipv6_forwarder && set_ip6tables_nat) die("'enable_ip6_forwarder' and 'set_ip6tables_nat' cannot be both true.", 0);
	if (enable_ipv4_forwarder && set_iptables_nat) die("'enable_ip_forwarder' and 'set_iptables_nat' cannot be both true.", 0);
}