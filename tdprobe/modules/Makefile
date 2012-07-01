CC=gcc
CFLAGS=-shared -fPIC -I .. -I ../../libtd -std=c99

all: dumpsize.so httprequests.so

clean:
	rm -f *.so

dumpsize.so: dumpsize.c
	$(CC) $(CFLAGS) $< -o $@
	
httprequests.so: httprequests.c
	$(CC) $(CFLAGS) $< -o $@
