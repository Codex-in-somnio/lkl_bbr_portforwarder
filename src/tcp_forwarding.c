#include "tcp_forwarding.h"
#include <errno.h>

bool ipv6_support;
in_addr_t target_ipv4_addr;
uint16_t listen_port;
uint16_t target_port;

struct bridge_lkl_args {
	int fd;
	int lkl_fd;
};

void *bridge_to_lkl(void *args)
{
	uint8_t buffer[BUFSIZE];
	int n_read = 0;
	
	struct bridge_lkl_args *args_ = args;
	int fd = args_ -> fd;
	int lkl_fd = args_ -> lkl_fd;
	int n_wrote, n_wrote_;

	while ((n_read = read(fd, buffer, BUFSIZE)) > 0)
	{
		n_wrote = 0;
		while (n_read > n_wrote)
		{
			n_wrote_ = lkl_sys_write(lkl_fd, buffer + n_wrote, n_read - n_wrote);
			if (n_wrote_ < 0) 
			{
				error("Error writing into client conn sock", errno);
				break;
			}
			n_wrote += n_wrote_;
		}
		if (n_wrote_ < 0) break;
	}
	if (n_read < 0) error("Error reading from target conn sock", errno);

	shutdown(fd, SHUT_RD);
	lkl_sys_shutdown(lkl_fd, SHUT_WR);
	
	close(fd);
		
	pthread_exit(NULL);
	return NULL;
}


void *bridge_from_lkl(void *args)
{
	uint8_t buffer[BUFSIZE];
	int n_read = 0;
	
	struct bridge_lkl_args *args_ = args;
	int fd = args_ -> fd;
	int lkl_fd = args_ -> lkl_fd;
	int n_wrote, n_wrote_;

	while ((n_read = lkl_sys_read(lkl_fd, buffer, BUFSIZE)) > 0)
	{
		n_wrote = 0;
		while (n_read > n_wrote)
		{
			n_wrote_ = write(fd, buffer + n_wrote, n_read - n_wrote);
			if (n_wrote_ < 0) 
			{
				error("Error writing into target conn sock", errno);
				break;
			}
			n_wrote += n_wrote_;
		}
		if (n_wrote_ < 0) break;
	}
	if (n_read < 0) error("Error reading from client conn sock", errno);

	lkl_sys_shutdown(lkl_fd, SHUT_RD);
	shutdown(fd, SHUT_WR);
	
	lkl_sys_close(lkl_fd);
	
	pthread_exit(NULL);
	return NULL;
}

void bridge_lkl(int fd, int lkl_fd)
{
	pthread_t from_lkl, to_lkl;
	struct bridge_lkl_args args = {.fd = fd, .lkl_fd = lkl_fd};
	
	int ret;
	
	ret = pthread_create(&from_lkl, NULL, bridge_from_lkl, (void *) &args);
	if(ret) error("Error creating thread", errno);
	
	ret = pthread_create(&to_lkl, NULL, bridge_to_lkl, (void *) &args);
	if(ret) error("Error creating thread", errno);
	
	pthread_join(from_lkl, NULL);
	pthread_join(to_lkl, NULL);	
}

void *tcp_forwarding(void *lkl_conn_fd_)
{
	int lkl_conn_fd = *((int *) lkl_conn_fd_);
	free(lkl_conn_fd_);
	
	struct sockaddr_in target_addr;
	target_addr.sin_family = AF_INET;
	target_addr.sin_port = htons(target_port);
	target_addr.sin_addr.s_addr = target_ipv4_addr;
	
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) error("Error opening forwarding socket.", errno);

	int conn_ret = connect(fd, (struct sockaddr *) & target_addr, sizeof(target_addr));
	if (conn_ret < 0) error("Error establishing connection to TCP forward target.", errno);

	printf("New connection to target established.\n");

	bridge_lkl(fd, lkl_conn_fd);
	
	pthread_exit(NULL);
	return NULL;
}

void tcp_forward_server()
{
	struct sockaddr_in6 addr6;
	struct sockaddr_in addr;
	struct lkl_sockaddr * saddr;
	int saddr_size;
	
	if (ipv6_support)
	{
		addr6.sin6_family = AF_INET6;
		addr6.sin6_port = htons(listen_port);
		addr6.sin6_addr = in6addr_any;
		
		saddr_size = sizeof(addr6);
		saddr = (struct lkl_sockaddr *) &addr6;
	}
	else
	{
		addr.sin_family = AF_INET;
		addr.sin_port = htons(listen_port);
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
		
		saddr_size = sizeof(addr);
		saddr = (struct lkl_sockaddr *) &addr;
	}

	int sockfd = lkl_sys_socket(ipv6_support ? LKL_AF_INET6 : LKL_AF_INET, LKL_SOCK_STREAM, 0);
	if (sockfd < 0) die("Error creating server socket", errno);
	
	int bind_ret = lkl_sys_bind(sockfd, saddr, saddr_size);
	if (bind_ret < 0) die("Error binding port.", errno);
	
	lkl_sys_listen(sockfd, 5);
	
	struct sockaddr client_addr;
	int len = sizeof(client_addr);

	while (1)
	{
		int connfd;
		connfd = lkl_sys_accept(sockfd, (struct lkl_sockaddr*) &client_addr, &len);
		if (connfd < 0) error("Error accepting connection.", errno);
		lkl_printf("Accepted new connection\n");
		
		pthread_t fwd;
		int *connfd_ = malloc(sizeof(int));
		*connfd_ = connfd;
		int ptc_ret = pthread_create(&fwd, NULL, tcp_forwarding, (void *) connfd_);
		if(ptc_ret) error("Error creating thread", ptc_ret);
	}
}