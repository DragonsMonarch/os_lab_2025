CC=gcc
CFLAGS=
TARGETS = tcp_server tcp_client udp_server udp_client

all: $(TARGETS)

# TCP COMPILATION
tcp_server: tcpserver.c
	$(CC) $(CFLAGS) -o $@ $<

tcp_client: tcpclient.c
	$(CC) $(CFLAGS) -o $@ $<

# UDP COMPILATION
udp_server: udpserver.c
	$(CC) $(CFLAGS) -o $@ $<

udp_client: udpclient.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS)

rebuild: clean all