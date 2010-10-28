CFLAGS=-g -Wall
check_nix: check_nix.c resolve.o resolve.h datefuncs.o getattributebyname.o getattributebyname.h ns.o
	$(CC) $(CFLAGS) -o check_nix check_nix.c resolve.o getattributebyname.o datefuncs.o ns.o -lresolv -lm

resolve.o: resolve.c resolve.h
datefuncs.o: datefuncs.c
getattributebyname.o: getattributebyname.c getattributebyname.h
ns.o: ns.c ns.h

clean:
	rm -f *.o
clobber: clean
	rm check_nix
