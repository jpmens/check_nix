# NAME

check\_nix - Nagios/Icinga plugin to check freshness of NiX Spam
DNSBL

# SYNOPSIS

check\_nix [-N *address*] [-D *domain*] [-w *warn seconds*] [-c
*critical seconds*] [-S *statusfile*]

# DESCRIPTION

(The German word "nichts", meaning "nothing", is often pronounced
as "nix" by non-native speakers.)

*NiX Spam* (i.e. "nothing spam" or "no spam") DNS black-list (or
block-list) created by Bert Ungerer of the German *ix* magazine.
Information on the *Nix Spam* DNSBL can be obtained at
<http://goo.gl/rOxd>. Mid October 2010 we requested, and graciously
got, a DNS TXT resource record as an
[RFC 1464](http://tools.ietf.org/html/rfc1464) string attribute
inserted into the zone apex. This TXT RR contains a "heartbeat" ISO
8601 timestamp which indicates the *approximate* time when the list
was last updated:

    60 IN TXT "heartbeat=2010-10-25T15:46:01+02:00"

We requested this timestamp so that zone slave servers can check
whether they are still approximately up to date with their zone
transfers; the DNSBL is updated several times per second and the
SOA serial number has no relationship to time. Note once again,
that the heartbeat reflects the *approximate* and not the exact
time of the last zone update (i.e. the heartbeat is updated once a
minute).

*check\_nix* is a Nagios/Icinga plugin which queries the timestamp
to check the freshness of a *Nix Spam* slave. During a zone
transfer, the master's time stamp is transferred along to the
zone's slave servers. Administrators on the slaves can now compare
that time stamp to their own server time and thus determine if zone
transfers are occurring in a timely fashion.

*check\_nix* does exactly that. It obtains the DNS TXT resource
record (RR) from a slave server and compares that to the system
time. If the difference is larger than *warn seconds* or
*critical seconds*, *check\_nix* issues an appropriate diagnostic
message and exits with a WARNING or CRITICAL code to inform the
administrator's monitoring interface that something is wrong.

# OPTIONS

*check\_nix* understands the following options. (The C version
currently supports the short options only.)

-D *domain*, --domain=*domain*
:   Specify the domain for which to look up the TXT record in the
    DNS. The default is `ix.dnsbl.manitu.net.`.

-N *nameserver*, --nameserver=*nameserver*
:   Specify which name server (IP or name) to use; default is
    `127.0.0.1`.

-w *seconds*, --warning=*seconds*
:   Number of seconds difference between exiting with a WARNING
    code; defaults to 600 seconds (10 minutes).

-c *seconds*, --critical=*seconds*
:   Number of seconds difference between exiting with a CRITICAL
    code; defaults to 1800 seconds (30 minutes).

-S *filename*, --statusfile=*filename*
:   The file into which *check\_nix* writes a verbose status code
    (i.e. `"OK"`) when it runs. See below. This file must be writeable
    by the *check\_nix* process; if it cannot create or open the file
    for writing, errors are silently ignored.
-d, --debug
:   C version only: enable debugging; not generally useful.


# STATUSFILE

When *check\_nix* runs, you can specify the name of a file into
which it writes a verbose status code (i.e. `"OK"`, `"WARNING"`,
...) to indicate the status of the last check. An external process
may monitor this file to do something clever, such as stop a
process, lock a firewall, etc. Note that this file must be writable
by the caller.

Example:

    $ rm /tmp/nix.status
    $ check_nix -S /tmp/nix.status
    DNSBL last updated on slave [192.168.1.20]  0 days, 00:00:20 ago
    $ cat /tmp/nix.status
    OK
    $

The reason this was implemented is that since a DNSBL can cause
e-mail to be blocked, we believe it is better to have a name server
*not* answer than answer incorrectly. In other words, what you may
wish to do is to have an external process periodically verify
whether the DNSBL slave is running smoothly, and if it isn't kill
off the name server until an operator has checked and fixed the
problem.

# BUGS

Yes.

If the clocks on the master and slave servers are askew in as much
as the master's clock is further than the slave's, results are
pretty unpredictable; *check\_nix* will currently return an OK
status.

We know of at least one platform on which `getaddrinfo`(3) behaves
incorrectly with a loopback address (`127.0.0.1`) and a running
`nscd`(8): if you experience difficulties using
`check_nix -N 127.0.0.1` (the default), either disable the latter
or use `localhost`.

# RETURN CODES

*check\_nix* exits with a code 0, 1, or 2 indicating a status of
OK, WARNING, or CRITICAL. A code of 3 (UNKNOWN) indicates a problem
during the DNS lookup, and the diagnostic message contains further
information.

# AVAILABILITY

<http://github.com/jpmens/check_nix>

# CREDITS

-   The C version of this product includes software developed by
    the Kungliga Tekniska Hvgskolan and its contributors.
-   The C version of this program contains date functions
    shamelessly swiped from
    <http://pleac.sourceforge.net/pleac_cposix/datesandtimes.html>

# AUTHOR

Jan-Piet Mens <http://mens.de>

# SEE ALSO

`resolver`(5).



