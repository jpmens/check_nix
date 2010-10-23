#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "resolve.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <resolv.h>
#include <regex.h> 


#define PORT	53

#define DOMAIN		"temp.aa"
#define ATTRIBUTE	"heartbeat"

#define TXT_RR	16

/*
 * Collect at most `ntxt` TXT RR into `txtlist[]`, and
 * return the count.
 */

int txt_rrs(char *txtlist[], int ntxt)
{
	struct dns_reply *r;
	struct resource_record *rr;
	int n = 0;

	if ((r = dns_lookup(DOMAIN, "TXT")) == NULL) {
		return (-1);
	}

	for (rr = r->head; rr; rr = rr->next) {
		if (rr->type == TXT_RR) {
			txtlist[n++] = strdup(rr->u.txt);
			txtlist[n] = NULL;
			if (n >= (ntxt-1)) {
				break;
			}
		}
	}

	dns_free_data(r);
	return (n);
}

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

char *findattribute(const char *attribute, char *txtlist[])
{
	regex_t preg; 
	int n, rc;
	size_t nmatch = 3; 
	regmatch_t pmatch[3]; 
	static char re[BUFSIZ], *ret = NULL;

	sprintf(re, "%s\\s*=\\s*(.*)", attribute);

        if ((rc = regcomp(&preg, re, REG_EXTENDED|REG_ICASE)) != 0) { 
                regerror(rc, &preg, re, sizeof(re)); 
                return (re);
        } 

        for (n = 0; txtlist[n]; n++) { 
                rc = regexec(&preg, txtlist[n], nmatch, pmatch, 0); 
                if (rc == 0) { 
                        // static char buf[1024]; 
                        regmatch_t *pm = &pmatch[1]; 

			ret = txtlist[n] + pm->rm_so;

                        // memcpy(buf, txtlist[n] + pm->rm_so, pm->rm_eo - pm->rm_so); 
                        // printf("\t-> MATCH (%d, %d) [%s]\n", 
                        //         (int)pm->rm_so, (int)pm->rm_eo, buf); 
                } else { 
                        // printf("\t%d\n", rc); 
                } 
        } 

        regfree(&preg); 
	return (ret);
}

#define _XOPEN_SOURCE /* glibc2 needs this */ 
#include <time.h> 

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


#define MAX_RRS		20

int main()
{
	char *txtlist[MAX_RRS];
	int count, n;
	char *datestring, fixdate[48];
	char buf[128]; 
	struct tm tm; 
	time_t tics, now; 
	void tics2comment(time_t old, time_t new, char *buf);

	time(&now); 


	setnameserver("127.0.0.1");


	count = txt_rrs(txtlist, MAX_RRS - 1);

	/*
	printf("count == %d\n", count);
	for (n = 0; n < count; n++) {
		printf("[%s]\n", txtlist[n]);
	}
	*/

	datestring = findattribute(ATTRIBUTE, txtlist);
	if (datestring == NULL) {
		printf("Can't find requested attribute\n");
		exit(1);
	}

	// printf("Gotit: %s\n", datestring);

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

#if 0
	printf("now  : %ld\n", now); 
	printf("tics : %ld\n", tics); 
	printf("diff : %ld\n", now - tics); 

	{
		struct tm *tmdiff;
		time_t diffsecs = now - tics;

		tmdiff = gmtime(&diffsecs);
		printf("Elapsed: %d days, %02d:%02d:%02d\n",
			tmdiff->tm_mday,
			tmdiff->tm_hour,
			tmdiff->tm_min,
			tmdiff->tm_sec);
	}

	strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S %Z", &tm); 

	printf("DNS   : %s\n", datestring); 
	printf("DNSfix: %s\n", fixdate); 
	printf("new   : %s\n", buf); 
	printf("ctime : %s", ctime(&tics)); 
#endif

	tics2comment(now, tics, buf);
	printf("DNSBL last updated on slave [XXX] %ld seconds --  %s\n", diffsecs, buf);


	for (n = 0; n < count; n++)
		free(txtlist[n]);
	return (0);
}
