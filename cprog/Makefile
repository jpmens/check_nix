CFLAGS=-g -Wall
check_nix: check_nix.c resolve.o resolve.h datefuncs.o
	$(CC) $(CFLAGS) -o check_nix check_nix.c resolve.o datefuncs.o -lresolv

resolve.o: resolve.c resolve.h
datefuncs.o: datefuncs.c

