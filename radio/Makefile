CC      = gcc
CFLAGS  = -std=gnu11 -Wall -Wextra -O2
LDLIBS  = -lssl -lcrypto

OBJS    = main.o http.o err.o tls.o

.PHONY: all clean

all: sikradio

sikradio: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDLIBS)

main.o: main.c http.h err.h utils.h tls.h
http.o: http.c http.h
err.o:  err.c  err.h
tls.o:  tls.c  tls.h err.h

clean:
	rm -f $(OBJS) sikradio
