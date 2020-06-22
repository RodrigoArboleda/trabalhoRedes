lib = libpthread.a
obj_client = cliente3.o utilities.o
obj_server = servidor3.o list.o utilities.o channel.o
bin_client = irc_client
bin_server = irc_server

CC = gcc
CPP_FLAGS = -Wall
C_FLAGS =
LD_FLAGS =

# Recipe

all: $(bin_client) $(bin_server)

server: $(bin_server)

client: $(bin_client)

$(bin_client): $(obj_client)
	$(CC) $(LD_FLAGS) $(obj_client) -l$(lib:lib%.a=%) -o $(bin_client) 

$(bin_server): $(obj_server)
	$(CC) $(LD_FLAGS) $(obj_server) -l$(lib:lib%.a=%) -o $(bin_server)

%.o: %.c
	$(CC) $(CPP_FLAGS) $(C_FLAGS) -c $<

run_server: $(bin_server)
	./$(bin_server)

run_client: $(bin_client)
	./$(bin_client)

run: $(bin_client)
	./$(bin_client)

.PHONY: clean

clean:
	rm -f $(obj_client)
	rm -f $(obj_server)
