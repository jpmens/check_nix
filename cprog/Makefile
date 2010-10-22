CFLAGS=-g -Wall
check_nix: check_nix.c resolve.o resolve.h
	$(CC) $(CFLAGS) -o check_nix check_nix.c resolve.o -lresolv

resolve.o: resolve.c resolve.h
