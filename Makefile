all: bankingClient bankingServer 

bankingClient: bankingClient.c
	$(CC) -o $@ $^ -pthread

bankingServer: bankingServer.c database.c
	$(CC) -o $@ $^ -pthread

clean:
	rm -f *.o bankingServer bankingClient
