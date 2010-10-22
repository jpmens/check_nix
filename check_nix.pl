#!/usr/bin/perl
# check_nix by Jan-Piet Mens <http://mens.de>


use strict;
use Net::DNS;
use DateTime::Format::ISO8601;
use Getopt::Long;
use Pod::Usage;
use vars qw($opt_v $opt_w $opt_c $opt_N $opt_D $opt_S $opt_h);

my %ERRORS=('OK'=>0,'WARNING'=>1,'CRITICAL'=>2,'UNKNOWN'=>3);
my @ERRS = qw(OK WARNING CRITICAL UNKNOWN);

my $warn_diff	= (10 * 60);
my $crit_diff	= (30 * 60);
my $nameserver	=  '127.0.0.1';
my $domain	= 'ix.dnsbl.manitu.net';
my $attribute	= 'heartbeat';
my $statusfile	= undef;

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

# print "diff: $diff_seconds\n";

my ($days, $hours, $minutes, $seconds) = $dur->in_units('days', 'hours', 'minutes', 'seconds');
printf "DNSBL last updated on slave [$nameserver] %d days, %02d:%02d:%02d ago\n",
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
		foreach my $a (@ans) {
			if ($a =~ /^$attribute\s*=\s*(.*)/i) {
				$txtdata = $1;
				last;
			}
			$msg = "attribute $attribute not found in DNS";
		}
	} else {
		$msg = "query for $domain TXT failed: " . $res->errorstring;
	}
	
	return ($txtdata, $msg);
}

sub terminate {
	my ($status) = @_;

	if (defined($statusfile) && open(STATUS, "> $statusfile")) {
		print STATUS $ERRS[$status], "\n";
		close STATUS;
	}
	exit($status);
}

sub do_args(){
	unless (GetOptions(
		"h" => \$opt_h, "help" => \$opt_h,
		"D:s" => \$opt_D, "domain:s" => \$opt_D,
		"N:s" => \$opt_N, "nameserver:s" => \$opt_N,
		"S:s" => \$opt_S, "statusfile:s" => \$opt_S,
		"w=i" => \$opt_w, "warning=i"  => \$opt_w,
		"c=i" => \$opt_c, "critical=i" => \$opt_c,
	)) {
		pod2usage(1);
		return 1;
	}

	pod2usage(1) if ($opt_h);

	$domain		= $opt_D if defined($opt_D);
	$nameserver	= $opt_N if defined($opt_N);
	$statusfile	= $opt_S if defined($opt_S);
	$warn_diff	= $opt_w if defined($opt_w);
	$crit_diff	= $opt_c if defined($opt_c);

	return 0;
}

__END__

=head1 SYNOPSIS

check_nix [-N I<address>] [-D I<domain>] [-w I<warn seconds>] [-c I<critical seconds>] [-S I<statusfile>]

