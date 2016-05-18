
default-target: echo-client.out echo-server.out

CC = clang
CC_FLAGS = -std=c99 -O2 -Wall -Wextra -Werror -pedantic-errors
LD = clang
LD_FLAGS = -static-libgcc -static -lpthread

%.o: %.c
	$(CC) -c $< -o $@ $(CC_FLAGS) -I .

%.out: %.o common.o
	$(LD) $^ -o $@ $(LD_FLAGS)

clean:
	$(RM) -f *.out *.o
.PHONY: clean


