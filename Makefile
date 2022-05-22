PROJ=/mnt/c/client-server-project/TicTacToe_Game_Server
ASIO=/mnt/c/client-server-project/asio-1.22.1
BIN=$(PROJ)/bin
SERVER=$(PROJ)/server
CLIENT=$(PROJ)/client
CC=g++
CPPFLAGS=-g -Wall -std=c++17 -I$(ASIO)/include 
LFLAGS=-lpthread


SERVER_OBJS=$(SERVER)/src/server_main.o

CLIENT_OBJS=$(CLIENT)/src/client_main.o 


all : $(BIN)/server $(BIN)/client 

$(BIN)/server : $(SERVER_OBJS)
	$(CC) $^ -o $@ $(LFLAGS)


$(BIN)/client : $(CLIENT_OBJS) 
	$(CC) $^ -o $@ $(LFLAGS)

%.o : %.c
	$(CC) -c $(CPPFLAGS) $< -o $@

clean:
	\rm -f $(BIN)/server $(BIN)/client
	\rm `find $(PROJ) -name '*.o'`
