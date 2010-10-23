CFLAGS=-g -Wall
check_nix: check_nix.c resolve.o resolve.h datefuncs.o getattributebyname.o getattributebyname.h
	$(CC) $(CFLAGS) -o check_nix check_nix.c resolve.o getattributebyname.o datefuncs.o -lresolv

resolve.o: resolve.c resolve.h
datefuncs.o: datefuncs.c
getattributebyname.o: getattributebyname.c getattributebyname.h

