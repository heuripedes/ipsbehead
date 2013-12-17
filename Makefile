
CFLAGS=-Wall -O2 -g -pedantic -fno-strict-aliasing

all: unix
unix: ipsbehead
win: ipsbehead.exe

clean:
	-rm -f ipsbehead ipsbehead.exe ipsbehead.o

ipsbehead: ipsbehead.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< 

ipsbehead.exe: ipsbehead.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< -lws2_32

