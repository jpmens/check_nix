/*
 * Jan-Piet Mens, October 2010
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "resolve.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <resolv.h>
#include <regex.h> 

#define TXT_RR	16

/*
 * This is a (half hearted) implementation of RFC 1464. `domain' is
 * the DNS domain to search for TXT resource records. `attribute'
 * is the name of the desired attribute, whose value will be copied
 * into at most `abuflen' characters of `abuf'. (`abuf' will be
 * null-terminated.)
 *
 * This implementation doesn't account for all the quoting magic
 * defined in RFC 1464. In particular, no unquoting is performed.
 * An attribute = value pair contains one `=' sign, optionally
 * surrounded by white space, which is ignored.
 *
 * getattributebyname() returns 1 if the desired attribute
 * was found, 0 otherwise. Note, that more than one TXT RR
 * might contain identical attributes, in which case the
 * first one retrieved in the DNS response is used.
 */

int
getattributebyname(char *domain, char *attribute, char *abuf, int abuflen)
{
	struct dns_reply *r;
	struct resource_record *rr;
	regex_t preg; 
	size_t nmatch = 3; 
	regmatch_t pmatch[3]; 
	static char re[BUFSIZ];
	int rc, ret = 0;


	if ((r = dns_lookup(domain, "TXT")) == NULL) {
		return (0);
	}

	sprintf(re, "%s\\s*=\\s*(.*)", attribute);

        if ((rc = regcomp(&preg, re, REG_EXTENDED|REG_ICASE)) != 0) { 
                regerror(rc, &preg, abuf, abuflen);
                return (0);
        } 

	for (rr = r->head; rr; rr = rr->next) {
		if (rr->type == TXT_RR) {
	                rc = regexec(&preg, rr->u.txt, nmatch, pmatch, 0); 
	                if (rc == 0) { 
	                        regmatch_t *pm = &pmatch[1]; 
	
				strncpy(abuf, rr->u.txt + pm->rm_so, abuflen);
				abuf[abuflen - 1] = 0;
				ret = 1;
				break;
	                } 
		}
	}

	dns_free_data(r);
        regfree(&preg); 
	return (ret);
}

#ifdef TESTING

int main(int argc, char **argv)
{
	int ret;
	char buf[1024];
	char *attribute = (argc == 2) ? argv[1] : "heartbeat";

	setnameserver("127.0.0.1");

	ret = getattributebyname("temp.aa", attribute, buf, sizeof(buf));

	if (ret == 1) {
		printf("Gotit: {%s}\n", buf);
	}

	return (0);
}
#endif
