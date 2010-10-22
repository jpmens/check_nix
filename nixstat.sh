#!/bin/sh
#(@)nixstat.sh
#
# Trivial example of how to query the --statusfile created by check_nix

STATUSFILE=/tmp/nix.stat

[ -r $STATUSFILE ] || { echo "$0: Can't open $STATUSFILE" >&2; exit 1; }

STATUS="`head -1 $STATUSFILE`"

case "$STATUS" in
	OK)
		echo "--- all ok"
		;;
	WARNING)
		echo "--- oopsie"
		;;
	CRITICAL)
		echo "--- oh, oh!"
		;;
	UNKNOWN)
		echo "--- somethin' unknown goin on here..."
		;;
	*)
		echo "$0: don't grok content of $STATUSFILE";
		exit 2;;
esac
