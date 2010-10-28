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
#include "ns.h"


#define DRIFT 2

#define DOMAIN		"ix.dnsbl.manitu.net."
#define ATTRIBUTE	"heartbeat"

#define NDECL(X) #X 

#define OK              (0) 
#define WARNING         (1) 
#define CRITICAL        (2) 
#define UNKNOWN         (3) 

#define DIM(x)	( sizeof(x) / sizeof(x[0]) )
static char *nagerr[] = { NDECL(OK), NDECL(WARNING), NDECL(CRITICAL), NDECL(UNKNOWN) };
static char *statusfile = NULL;

int debug = 0;

#define nagcode_to_string(x) ( (x) <= DIM(nagerr) ? nagerr[(x)] : "NULL" ) 

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

/* 
 * Convert an ISO 8601 timestamp of the form 
 * "2010-10-13T20:56:32+0200" (note there is no colon
 * in the timezone [+-]hhmm part) into the (struct tm) 
 * and (time_t) pointers supplied. 
 */ 

int iso8601_tm(const char *tstring, struct tm *tm, time_t *tics) 
{ 
        memset(tm, 0, sizeof(struct tm)); 
        strptime(tstring, "%FT%T%z", tm); 

	tm->tm_isdst = -1;	// FIXME?

        *tics = mktime(tm);
        if (*tics == (time_t)-1) 
                return (0); 

        // FIXME localtime_r(tics, tm); 

        return (1); 
} 

int main(int argc, char **argv)
{
	int rc, c;
	char datestring[BUFSIZ], fixdate[48], msg[BUFSIZ], buf[BUFSIZ];
	struct tm tm; 
	time_t tics, now; 
	void tics2comment(time_t old, time_t new, char *buf);
	char *domain = DOMAIN, *progname = *argv;
	char *ns = "127.0.0.1";
	time_t warn = 10 * 60;
	time_t crit = 30 * 60;

	while (1) {
		static struct option long_opts[] = {
		{ "nameserver", 1, 0, 'N' },
		{ "domain",     1, 0, 'D' },
		{ "warning",    1, 0, 'w' },
		{ "critical",   1, 0, 'c' },
		{ "statusfile", 1, 0, 'S' },
		{ "debug",      0, 0, 'd' },
		{ NULL,    0, 0, 0 }
		};
		int oix;

		if ((c = getopt_long(argc, argv, "N:D:w:c:S:d", long_opts, &oix)) == -1)
			break;

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
			case 'd':
				debug = 1;
				break;
			default:
				fprintf(stderr, "Usage: %s [-d] [-N address] [-D domain] [-w warn seconds] [-c critical seconds] [-S statusfile]\n", progname); 
				exit(127); 
		}
	}

	rc = setnameserver(ns, debug);
	if (rc == 0) {
		sprintf(msg, "Can't find name server for %s", ns);
		terminate(UNKNOWN, msg);
	}

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

	time(&now); 
	time_t diffsecs = now - tics;
	diffsecs = (diffsecs - DRIFT < 0) ? 0 : diffsecs;

	if (debug) {
		printf("datestring from DNS: %s\n", datestring);
		printf("fixed string x  DNS: %s\n", fixdate);
		printf("ctime now          : %s", ctime(&now));
		printf("tics now           : %ld\n", now);
		printf("tics from DNS date : %ld\n", tics);
		printf("tics difference    : %ld\n", diffsecs);
	}

	tics2comment(now, tics, buf);
	sprintf(msg, "last updated on slave [%s] %ld seconds (%s) ago",
			ns, diffsecs, buf);

	if (diffsecs > crit)
		terminate(CRITICAL, msg);
	else if (diffsecs > warn)
		terminate(WARNING, msg);

	terminate (OK, msg);
	/*notreached*/
	return 0;
}
