
CFLAGS=-Wall -O2 -g -pedantic

ipsbehead: ipsbehead.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< 

