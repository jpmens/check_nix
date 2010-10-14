#!/usr/bin/perl
# check_nix by Jan-Piet Mens <http://mens.de>


use strict;
use Net::DNS;
use DateTime::Format::ISO8601;
use Getopt::Long;
use vars qw($opt_v $opt_w $opt_c $opt_N $opt_D $opt_S);

my %ERRORS=('OK'=>0,'WARNING'=>1,'CRITICAL'=>2,'UNKNOWN'=>3);
my @ERRS = qw(OK WARNING CRITICAL UNKNOWN);

my $warn_diff	= 30;
my $crit_diff	= 60;
my $nameserver	=  '192.168.1.20';
my $domain	= '0.0.0.0.ix.dnsbl.manitu.net';
my $semafile	= undef;

Getopt::Long::Configure('bundling');
my $status = do_args();
if ($status){
	print "ERROR: processing arguments\n";
	terminate( $ERRORS{"UNKNOWN"} );
}

my ($tstamp, $msg) = TXT($nameserver, $domain);

unless (defined($tstamp)) {
	print "DNSBL $msg\n";
	terminate( $ERRORS{'UNKNOWN'} );
}

my $slavedt = DateTime::Format::ISO8601->parse_datetime($tstamp);
my $here = DateTime->now;
my $dur = $here->subtract_datetime($slavedt);

my $slave_secs = $slavedt->epoch();

my $diff_seconds = $here->epoch - $slavedt->epoch;

my ($days, $hours, $minutes, $seconds) = $dur->in_units('days', 'hours', 'minutes', 'seconds');
printf "DNSBL last updated on slave [$nameserver]  %d days, %02d:%02d:%02d ago\n",
		$days, $hours, $minutes, $seconds;

if ($diff_seconds > $crit_diff) {
	terminate( $ERRORS{'CRITICAL'} );
} elsif ($diff_seconds > $warn_diff) {
	terminate( $ERRORS{'WARNING'} );
}
terminate( $ERRORS{'OK'} );


# Resolve the specified domain name and hopefully return
# the rdata of the single TXT record, which contains an
# ISO8601 date/time string as in '2010-10-13T18:56:32'.

sub TXT {
	my ($nameserver, $domain) = @_;
	my ($txtdata, $msg) = (undef, '');

	my $res = Net::DNS::Resolver->new(
		nameservers	=> [ $nameserver ],
		recurse		=> 0,
		debug		=> 0,
		tcp_timeout	=> 5,
		udp_timeout	=> 5,
		);

	my $query = $res->query($domain, 'TXT');
	if ($query) {
		my @ans = map { $_ -> txtdata } (grep {$_->type eq 'TXT'} $query->answer);
		$txtdata = @ans[0];
	} else {
		$msg = "query for $domain TXT failed: " . $res->errorstring;
	}
	
	return ($txtdata, $msg);
}

sub terminate {
	my ($status) = @_;

	if (defined($semafile) && open(SEMA, "> $semafile")) {
		print SEMA $ERRS[$status], "\n";
		close SEMA;
	}
	exit($status);
}

sub do_args(){
	unless (GetOptions(
		"D:s" => \$opt_D, "domain:s" => \$opt_D,
		"N:s" => \$opt_N, "nameserver:s" => \$opt_N,
		"S:s" => \$opt_S, "semaphore:s" => \$opt_S,
		"w=i" => \$opt_w, "warning=i"  => \$opt_w,
		"c=i" => \$opt_c, "critical=i" => \$opt_c,
	)) {
		print STDERR "Usage: $0 [-D domain] [-N address] [-w seconds] [-c seconds] [-S semaphorefile]\n";
		return 1;
	}

	$domain		= $opt_D if defined($opt_D);
	$nameserver	= $opt_N if defined($opt_N);
	$semafile	= $opt_S if defined($opt_S);
	$warn_diff	= $opt_w if defined($opt_w);
	$crit_diff	= $opt_c if defined($opt_c);

	return 0;
}

__END__

=head1 NAME

B<check_nix> - Nagios/Icinga checker for freshness of ix.dnsbl.manitu.net DNSBL

=head1 SYNOPSIS

check_nix [-N I<address>] [-D I<domain>] [-w I<warn seconds>] [-c I<critical seconds>] [-S I<semaphore>]

=head1 DESCRIPTION

The name B<check_nix> is a pun. The German word "nichts", meaning "nothing", is often
pronounced as "nix", either by foreign speakers or jokingly. Does B<check_nix> check
nothing? No, it doesn't. It checks the correct zone transfer of the I<Nix Spam> (i.e.
"nothing spam" or "no spam") DNS black-list (or block-list) created by
Bert Ungerer of the German I<ix> magazine. Information on the I<Nix Spam> DNSBL
can be obtained at L<http://goo.gl/rOxd>.

Since mid October 2010, the Nix Spam DNSBL carries a specific record in it
denoting the ISO 8601 time of the server (i.e. of the master zone). This approximate
time stamp can be used to determine how fresh a DNS slave of the zone is.

After a zone transfer, the master's time stamp is transferred along to the zone's
slave servers. Administrators on the slaves can now compare that time stamp to
their own server time and thus determine if zone transfers are occurring in a 
timely fashion.

B<check_nix> does exactly that. It obtains the DNS TXT resource record (RR) from
a slave server and compares that to the system time. If the difference is larger
than I<warn seconds> or I<critical seconds>, B<check_nix> issues an appropriate
diagnostic message and exits with a WARNING or CRITICAL code to inform the
administrator's monitoring interface that something is wrong.

=head2 Options

=over 12

=item I<-D> or I<--domain>

Specify the domain for which to look up the TXT record in the DNS. Default is C<0.0.0.0.ix.dnsbl.manitu.net>.

=item I<-N> or I<--nameserver>

Specify which name server (IP or name) to use; default is C<127.0.0.1>.

=item I<-w> or I<--warning>

Number of seconds difference between exiting with a WARNING code.

=item I<-c> or I<--critical>

Number of seconds difference between exiting with a CRITICAL code.

=item I<-S> or I<--semaphore>

Specify a file name (no default) into which B<check_nix> writes a verbose status code (i.e. C<"OK">, C"<WARNING>", ...) to indicate the status of the last check. An external process may monitor this file to do something clever, such as stop a process, lock a firewall, etc. Note that this file must be writable by the caller.

=back

=head1 BUGS

Yes.

If the clocks on the master and slave servers are askew in as much as the master's
clock is further than the slave's, results are pretty 
unpredictable; B<check_nix> will currently return an OK status.

=head1 RETURN CODES

B<check_nix> exits with a code 0, 1, or 2 indicating a status of OK, WARNING, or
CRITICAL. A code of 3 (UNKNOWN) indicates a problem during the DNS lookup, and 
the diagnostic message contains further information.

=head1 AVAILABILITY



=head1 AUTHOR

Jan-Piet Mens L<http://mens.de>
