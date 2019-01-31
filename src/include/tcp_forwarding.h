#ifndef _TCP_FORWARDING_H
#define _TCP_FORWARDING_H

#include "common.h"
#include "util.h"

#include <sys/socket.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include <pthread.h>

#include <lkl.h>
#include <lkl_host.h>

#define BUFSIZE 4096

void tcp_forward_server();

#endif