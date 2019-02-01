#include "ip_forwarder.h"

int tap_ifindex;
uint8_t lkl_mac_addr[6];
uint8_t tap_mac_addr[6];
struct in6_addr listen_ipv6_addr;
struct in6_addr lkl_ipv6_addr;
in_addr_t listen_ipv4_addr;
in_addr_t lkl_ipv4_addr;
char host_if_name_v6[10];
char host_if_name_v4[10];
uint16_t listen_port;

void recalc_checksum(uint8_t *packet, int len, int hdr_len, bool ipv6)
{

	struct { 
		uint8_t src_ip[16];
		uint8_t dst_ip[16];
		uint8_t len[2];
		uint8_t zeros[3];
		uint8_t next_header;
	} pseudo_header_v6 = {0};
	
	struct { 
		uint8_t src_ip[4];
		uint8_t dst_ip[4];
		uint8_t zeros;
		uint8_t next_header;
		uint8_t len[2];
	} pseudo_header_v4 = {0};

	if (ipv6)
	{
		pseudo_header_v6.next_header = 6;
		memcpy(&pseudo_header_v6.src_ip, packet + 8, 16);
		memcpy(&pseudo_header_v6.dst_ip, packet + 24, 16);
		memcpy(&pseudo_header_v6.len, packet + 4, 2);
	}
	else
	{
		pseudo_header_v4.next_header = 6;
		memcpy(&pseudo_header_v4.src_ip, packet + 12, 4);
		memcpy(&pseudo_header_v4.dst_ip, packet + 16, 4);
		memcpy(&pseudo_header_v4.len, packet + 2, 2);
		int tcp_len = ((packet[2] << 8) | packet[3]) - hdr_len;
		pseudo_header_v4.len[0] = (tcp_len >> 8);
		pseudo_header_v4.len[1] = tcp_len & 0xff;
	}
	
	uint8_t *pseudo_header = (ipv6 ? (uint8_t *) &pseudo_header_v6 : (uint8_t *) &pseudo_header_v4);
	int phdr_len = ipv6 ? sizeof(pseudo_header_v6) : sizeof(pseudo_header_v4);
	
	len -= hdr_len;
	uint8_t *packet_tcp = packet + hdr_len;
	
	for (int do_v4_hdr = 0; do_v4_hdr < 2; do_v4_hdr++)
	{
		uint64_t sum = 0;
		if (do_v4_hdr)
		{
			if (ipv6) continue;
			
			packet[10] = 0;
			packet[11] = 0;
			
			for (int i = 0; i < hdr_len; i += 2)
			{
				sum += (packet[i] << 8) | packet[i + 1];
			}
		}
		else
		{
			packet_tcp[16] = 0;
			packet_tcp[17] = 0;
			
			for (int i = 0; i < phdr_len; i += 2)
			{
				sum += (pseudo_header[i] << 8) | pseudo_header[i + 1];
			}
			for (int i = 0; i < len - 1; i += 2)
			{
				sum += (packet_tcp[i] << 8) | packet_tcp[i + 1];
			}
			if (len & 1)
			{
				sum += (packet_tcp[len - 1] << 8);
			}
		}

		sum = (sum & 0xffff) + (sum >> 16);
		sum += (sum >> 16);
		
		sum = 0xffff & ~sum; 

		if (do_v4_hdr)
		{
			packet[10] = sum >> 8;
			packet[11] = sum & 0xff;
		}
		else
		{
			packet_tcp[16] = sum >> 8;
			packet_tcp[17] = sum & 0xff;
		}
	}
}

void *ip_forwarder(void *args)
{
	bool ipv6 = (bool) args;
	
	puts(ipv6 ? "Starting internal IPv6 forwarder." : "Starting internal IPv4 forwarder.");
	
	int raw_socket = socket(AF_PACKET, SOCK_RAW, htons(ipv6 ? ETH_P_IPV6 : ETH_P_IP));
	int cooked_socket = socket(AF_PACKET, SOCK_DGRAM, htons(ipv6 ? ETH_P_IPV6 : ETH_P_IP));
	if(raw_socket < 0) die("ip_forwarder: raw_socket", errno);
	if(cooked_socket < 0) die("ip_forwarder: cooked_socket", errno);

	
	struct sockaddr_ll sall_lkl;
	sall_lkl.sll_family = AF_PACKET;
	sall_lkl.sll_ifindex = tap_ifindex;
	sall_lkl.sll_halen = ETH_ALEN;
	memcpy(&sall_lkl.sll_addr, &lkl_mac_addr, 6);
	
	struct sockaddr_ll sall_client;
	sall_client.sll_family = AF_PACKET;
	sall_client.sll_ifindex = get_ifindex(ipv6 ? host_if_name_v6 : host_if_name_v4);
	sall_client.sll_protocol = htons(ipv6 ? ETH_P_IPV6 : ETH_P_IP);
	
	struct sockaddr *target_saddr;
	int target_saddr_size = target_saddr_size = sizeof(struct sockaddr_ll);
	
	struct in6_addr host_addr6, lkl_addr6;
	uint32_t host_addr4, lkl_addr4;

	if (ipv6)
	{
		memcpy(&lkl_addr6, &lkl_ipv6_addr, sizeof(struct in6_addr));
		memcpy(&host_addr6, &listen_ipv6_addr, sizeof(struct in6_addr));
	}
	else
	{
		lkl_addr4 = lkl_ipv4_addr;
		host_addr4 = listen_ipv4_addr;
	}
	
	uint8_t buffer[BUFSIZE + 14];
	uint8_t *read_buffer = buffer + 14;
	uint8_t *write_buffer;
	int n_read = 0;
	int write_buffer_length = 0;
	int write_socket;
	
	while ((n_read = read(raw_socket, read_buffer, BUFSIZE)) > 0)
	{
		int hdr_len_in, hdr_len_out;
		
		if (ipv6)
		{
			hdr_len_in = 40;
			hdr_len_out = 40;
		}
		else
		{
			hdr_len_in = (read_buffer[0] & 0xf) * 4;
			hdr_len_out = (read_buffer[14] & 0xf) * 4;
		}
		
		uint16_t incoming_dest_port = (read_buffer[hdr_len_in + 2] << 8) | read_buffer[hdr_len_in + 3];
		uint16_t outgoing_src_port = (read_buffer[hdr_len_out + 14] << 8) | read_buffer[hdr_len_out + 1 + 14];
		uint16_t outgoing_packet_type = (read_buffer[12] << 8) | read_buffer[13];
		
		uint8_t *incoming_dest_addr6, *outgoing_src_addr6;
		uint32_t incoming_dest_addr4, outgoing_src_addr4;
		
		if (ipv6)
		{
			incoming_dest_addr6 = read_buffer + 24;
			outgoing_src_addr6 = read_buffer + 8 + 14;
		}
		else
		{
			incoming_dest_addr4 = ntohl((read_buffer[16] << 24) | (read_buffer[17] << 16) | (read_buffer[18] << 8) | read_buffer[19]);
			outgoing_src_addr4 = ntohl((read_buffer[12 + 14] << 24) | (read_buffer[13 + 14] << 16) | (read_buffer[14 + 14] << 8) | read_buffer[15 + 14]);
		}
		
		write_buffer_length = 0;
		
		bool in = (ipv6 ? (memcmp(incoming_dest_addr6, &host_addr6, 16) == 0) : (incoming_dest_addr4 == host_addr4)) && incoming_dest_port == listen_port;
		bool out = (ipv6 ? (memcmp(outgoing_src_addr6, &lkl_addr6, 16) == 0) : (outgoing_src_addr4 == lkl_addr4)) && outgoing_src_port == listen_port && outgoing_packet_type == (ipv6 ? 0x86dd : 0x0800);
		
		if (out)
		{
			write_buffer_length = n_read - 14;
			write_buffer = read_buffer + 14;
			
			uint16_t checksum_orig = (read_buffer[hdr_len_out + 16 + 14] << 8) | read_buffer[hdr_len_out + 17 + 14];
			recalc_checksum(read_buffer + 14, n_read - 14, hdr_len_out, ipv6);
			uint16_t checksum_calculated = (read_buffer[hdr_len_out + 16 + 14] << 8) | read_buffer[hdr_len_out + 17 + 14];
			bool bad_checksum = false;
			if (checksum_calculated != checksum_orig)
			{
				printf("orig: %04x, actual: %04x", checksum_orig, checksum_calculated);
				error("Outgoing TCP checksum failed. Packet droped.", false);
				print_hex(read_buffer, n_read);
				continue;
			}
			
			if (ipv6)
			{
				memcpy(outgoing_src_addr6, &host_addr6, 16);
			}
			else
			{
				uint32_t host_addr4_n = htonl(host_addr4);
				for (int i = 0; i < 4; ++i) write_buffer[12 + i] = (host_addr4_n >> (24 - 8 * i)) & 0xff;
			}
			recalc_checksum(write_buffer, write_buffer_length, hdr_len_out, ipv6);
			
			write_socket = cooked_socket;
			target_saddr = (struct sockaddr *) &sall_client;
		}
		else if (in)
		{
			write_buffer_length = n_read + 14;
			write_buffer = read_buffer - 14;
			
			uint16_t checksum_orig = (read_buffer[hdr_len_in + 16] << 8) | read_buffer[hdr_len_in + 17];
			recalc_checksum(read_buffer, n_read, hdr_len_in, ipv6);
			uint16_t checksum_calculated = (read_buffer[hdr_len_in + 16] << 8) | read_buffer[hdr_len_in + 17];
			bool bad_checksum = false;
			if (checksum_calculated != checksum_orig)
			{
				printf("orig: %04x, actual: %04x", checksum_orig, checksum_calculated);
				error("Incoming TCP checksum failed. Packet droped.", false);
				print_hex(read_buffer, n_read);
				continue;
			}
			
			if (ipv6)
			{
				memcpy(incoming_dest_addr6, &lkl_addr6, 16);
			
				write_buffer[12] = 0x86;
				write_buffer[13] = 0xdd;
			}
			else
			{
				uint32_t lkl_addr4_n = htonl(lkl_addr4);
				for (int i = 0; i < 4; ++i) write_buffer[16 + 14 + i] = (lkl_addr4_n >> (24 - 8 * i)) & 0xff;
				
				write_buffer[12] = 0x08;
				write_buffer[13] = 0x00;			
			}
			recalc_checksum(write_buffer + 14, write_buffer_length - 14, hdr_len_in, ipv6);
			
			memcpy(write_buffer + 6, &tap_mac_addr, 6);
			memcpy(write_buffer, &lkl_mac_addr, 6);
			
			target_saddr = (struct sockaddr *) &sall_lkl;
			write_socket = raw_socket;
		}
		if (write_buffer_length > 0)
		{
			int n_wrote = 0;
			while (write_buffer_length > n_wrote)
			{
				int n_wrote_ = sendto(write_socket, write_buffer + n_wrote, write_buffer_length - n_wrote, 0, target_saddr, target_saddr_size);
				if (n_wrote_ < 0) 
				{
					error("Error writing into raw sock for ip forwarding.", errno);
					break;
				}
				n_wrote += n_wrote_;
			}
		}
	}

	pthread_exit(NULL);
	return NULL;
}

void start_ip_forwarder(bool ipv6)
{
	pthread_t ip_fwd_thread;
	int ptc_ret = pthread_create(&ip_fwd_thread, NULL, ip_forwarder, (void *) ipv6);
	if(ptc_ret) die("IP forwarder: error creating thread", ptc_ret);
}
