/*
 * Set up resolver to use specified DNS server in
 * `srv' (name or IP)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include "ns.h"

#define NAMESERVER_PORT	53

void setnameserver(char *srv)
{
	short port = htons(NAMESERVER_PORT);
	struct __res_state res_t, res;
	int nscount = 0;
	union res_sockaddr_union u[MAXNS];
	struct addrinfo *answer = NULL, *cur = NULL, hint;

	res_ninit(&res);
	res_t = res;

	memset(u, 0, sizeof(u));
	res.pfcode = 0;
	res.options = RES_DEFAULT;
	memset(&hint, 0, sizeof(hint));
	hint.ai_socktype = SOCK_DGRAM;

	/* Need default resolver info for getaddrinfo() to find names */
	if (!getaddrinfo(srv, NULL, &hint, &answer)) {
		res = res_t;
		cur = answer;
		for (cur = answer; cur != NULL; cur = cur->ai_next) {
			if (nscount == MAXNS)
				break;
			switch (cur->ai_addr->sa_family) {
				case AF_INET6:
					u[nscount].sin6 = *(struct sockaddr_in6*)cur->ai_addr;
					u[nscount++].sin6.sin6_port = port;
					break;
				case AF_INET:
					u[nscount].sin = *(struct sockaddr_in*)cur->ai_addr;
					u[nscount++].sin.sin_port = port;
				break;
			}
		}
		if (nscount != 0)
			res_setservers(&res, u, nscount);
		freeaddrinfo(answer);
	} else {
		res = res_t;
		fprintf(stderr, "Bad server: %s\n", srv);
	}

	/* Set up resolver to use our name server on subsequent queries */
	_res = res;
}

#ifdef HISTORIC_OLD_STUFF_OF_MINE
void setnameserver(const char *addr)
{
	struct sockaddr_in ns[1];

	res_init();

	ns[0].sin_addr.s_addr = inet_addr(addr);
	ns[0].sin_family = AF_INET;
	ns[0].sin_port = htons(PORT);
	ns[0].sin_len = sizeof(struct sockaddr_in);

	_res.nsaddr_list[0] = ns[0];
}
#endif
