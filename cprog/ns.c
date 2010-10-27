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

int setnameserver(char *srv, int debug) 
{ 
	struct addrinfo *ans, *r, hint; 
	int s; 
	void *addr; 
	char *ipver, ipstr[INET6_ADDRSTRLEN]; 
	struct sockaddr_in ns_list[MAXNS]; 
	int nscount = 0, n; 

	memset(&hint, 0, sizeof (struct addrinfo)); 
	hint.ai_family = AF_INET; 
	// hint.ai_family = AF_UNSPEC; 
	hint.ai_socktype = SOCK_DGRAM; 

	if (debug) {
		fprintf(stderr, "Find address for srv == %s\n", srv);
	}

	res_init(); 

	/* 
	 * Don't mess with resolver config as yet -- getaddrinfo() 
	 * needs default resolver to lookup names. 
	 */ 

	if ((s = getaddrinfo(srv, "domain", &hint, &ans)) != 0) { 
		fprintf(stderr, "Can't get address info for %s: %s\n", 
			srv, gai_strerror(s)); 
		return (0);
	} 

	for (r = ans; r && (nscount < MAXNS); r = r->ai_next) { 
#if 0 
		char host[NI_MAXHOST], service[NI_MAXSERV]; 
		s = getnameinfo(r->ai_addr, r->ai_addrlen, 
				host, NI_MAXHOST, service, NI_MAXSERV, NI_NUMERICSERV); 
		if (s == 0) { 
			fprintf(stderr, "\thost %s:%s\n", host, service); 
		} else { 
			fprintf(stderr, "getnameinfo: %s\n", gai_strerror(s)); 
		} 
#endif 
		if (r->ai_family == AF_INET) { 
			struct sockaddr_in *ipv4 = (struct sockaddr_in *)r->ai_addr; 
			struct sockaddr_in *nsp = &ns_list[nscount]; 

			addr = &(ipv4->sin_addr); 
			ipver = "IPv4"; 

			memset(nsp, 0, sizeof(struct sockaddr_in)); 

			nsp->sin_addr.s_addr    = ipv4->sin_addr.s_addr; 
			nsp->sin_port	   = ipv4->sin_port; 
			nsp->sin_family	 = ipv4->sin_family; 

			nscount++; 

		} else if (r->ai_family == AF_INET6) { 
			struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)r->ai_addr; 

			addr = &(ipv6->sin6_addr); 
			ipver = "IPv4"; 

			fprintf(stderr, "IPv6 addresses unsupported\n"); 
			// FIXME 
		} 

		if (debug) {
			inet_ntop(r->ai_family, addr, ipstr, sizeof ipstr); 
			fprintf(stderr, "  %s: %s\n", ipver, ipstr); 
		}

	} 
	freeaddrinfo(ans); 

	if (nscount > 0)  {

		/* 
		 * We have a "copy" of required name server addresses in 
		 * ns_list[]; copy that into _res and set counter. 
		 */ 

		for (n = 0; n < nscount; n++) { 
			_res.nsaddr_list[n] = ns_list[n]; 
		} 
		_res.nscount = nscount; 

		_res.options &= ~RES_RECURSE; 
		_res.options |= RES_INIT; 
		// _res.options |= RES_DEBUG; 
		_res.options &= ~(RES_DNSRCH | RES_DEFNAMES); 
	}
	
	return (nscount);
}
