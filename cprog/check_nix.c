#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "resolve.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <resolv.h>
#include <regex.h> 
#define _XOPEN_SOURCE /* glibc2 needs this */ 
#include <time.h> 
#include "getattributebyname.h"
#include <getopt.h>


#define PORT	53

#define DOMAIN		"ix.dnsbl.manitu.net"
#define ATTRIBUTE	"heartbeat"

#define NDECL(X) #X 

#define OK              (0) 
#define WARNING         (1) 
#define CRITICAL        (2) 
#define UNKNOWN         (3) 

static char *nagerr[] = { NDECL(OK), NDECL(WARNING), NDECL(CRITICAL), NDECL(UNKNOWN), NULL }; 
static char *statusfile = NULL;

#define nagcode_to_string(x) ( (x) <= (UNKNOWN) ? nagerr[(x)] : "NULL" ) 

void terminate(int code, char *reason)
{
	FILE *fp;

	if (statusfile && ((fp = fopen(statusfile, "w")) != NULL)) {
		fprintf(fp, "%s\n", nagcode_to_string(code));
		fclose(fp);
	}
	printf("DNSBL %s\n", reason);
	exit(code);
}

#define TXT_RR	16

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


#define TRUE 1 
#define FALSE 0 

/* 
 * Convert an ISO 8601 timestamp of the form 
 * "2010-10-13T20:56:32+0200" (note there is no colon
 * in the timezone [+-]hhmm part) into the (struct tm) 
 * and (time_t) pointers supplied. 
 */ 

int iso8601_tm(const char *tstring, struct tm *tm, time_t *tics) 
{ 
        // FIXME 
	// tzset(); 
	// tzsetwall(); 

        memset(tm, 0, sizeof(struct tm)); 
        strptime(tstring, "%FT%T%z", tm); 

        *tics = mktime(tm);
        if (*tics == (time_t)-1) 
                return (FALSE); 

        localtime_r(tics, tm); 

        return (TRUE); 
} 

int main(int argc, char **argv)
{
	int rc, c;
	char datestring[BUFSIZ], fixdate[48], msg[BUFSIZ];
	char buf[128]; 
	struct tm tm; 
	time_t tics, now; 
	void tics2comment(time_t old, time_t new, char *buf);
	char *domain = DOMAIN;
	char *ns = "127.0.0.1";
	time_t warn = 10 * 60;
	time_t crit = 30 * 60;

	while ((c = getopt(argc, argv, "N:D:w:c:S:")) != EOF) {
		switch (c) {
			case 'N':
				ns = strdup(optarg);
				break;
			case 'D':
				domain = strdup(optarg);
				break;
			case 'S':
				statusfile = strdup(optarg);
				break;
			case 'w':
				warn = atol(optarg);
				break;
			case 'c':
				crit = atol(optarg);
				break;
			default:
				puts("USAGE");
				exit(2);
		}
	}


	time(&now); 

	// FIXME:
	//	disable resolv.conf expansion
	setnameserver(ns);

	rc = getattributebyname(domain, ATTRIBUTE, datestring, sizeof(datestring));
	if (rc == 0) {
		sprintf(msg, "Can't find %s attribute in %s", ATTRIBUTE, domain);
		terminate(UNKNOWN, msg);
	}

	/* I have to "fix" the [+-]hh:mm of the time-zone modifier; we
	 * are getting, say, +02:00, and I have to remove the colon
	 * for strptime(3) to function correctly.
	 */

	strcpy(fixdate, datestring);
	fixdate[strlen(fixdate) - 3] = fixdate[strlen(fixdate) - 2];
	fixdate[strlen(fixdate) - 2] = fixdate[strlen(fixdate) - 1];
	fixdate[strlen(fixdate) - 1] = 0;
	iso8601_tm(fixdate, &tm, &tics); 

	time_t diffsecs = now - tics;

	tics2comment(now, tics, buf);
	sprintf(msg, "last updated on slave [%s] %ld seconds --  %s",
			ns, diffsecs, buf);

	if (diffsecs > crit)
		terminate(CRITICAL, msg);
	else if (diffsecs > warn)
		terminate(WARNING, msg);

	terminate (OK, msg);
	/*notreached*/
	return 0;
}
