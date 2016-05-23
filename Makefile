
tcp		:= 		echo-server.out echo-client.out
ssl		:=		ssl-server.out ssl-client.out
udp		:= 		datagram-server.out datagram-client.out

default-target: $(tcp) $(ssl) $(udp)

CC = clang
CC_FLAGS = -std=c99 -O2 -Wall -Wextra -Werror -pedantic-errors
LD = clang
LD_FLAGS = -static-libgcc -lpthread -lssl -lcrypto -ldl -lc

%.o: %.c
	$(CC) -c $< -o $@ $(CC_FLAGS) -I .

%.out: %.o common.o
	$(LD) $^ -o $@ $(LD_FLAGS)

generate: openssl.cnf
	openssl genrsa -out private-key.pem 2048
	openssl req -new -x509 -key private-key.pem -out cacert.pem -days 365 -config $<

clean:
	$(RM) -f *.out *.o
.PHONY: clean


