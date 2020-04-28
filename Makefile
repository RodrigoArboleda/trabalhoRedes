lib = libpthread.a
obj = main.o cliente.o servidor.o menssage.o utilities.o
bin = irc

CC = gcc
CPP_FLAGS = -Wall -ansi
C_FLAGS =
LD_FLAGS =

# Recipe

all: $(bin)

$(bin): $(obj)
	$(CC) $(LD_FLAGS) $(obj) -l$(lib:lib%.a=%) -o $(bin) 


%.o: %.c
	$(CC) $(CPP_FLAGS) $(C_FLAGS) -c $<

.PHONY: clean

clean:
	rm -f $(bin) $(obj)r
