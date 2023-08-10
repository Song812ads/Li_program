all: client server

client: TCP_client.o
	cc TCP_client.o -o client

server: TCP_server.o
	cc TCP_server.o -o server

TCP_client.o: TCP_client.c
	cc -c TCP_client.c -o TCP_client.o

TCP_server.o: TCP_server.c
	cc -c TCP_server.c -o TCP_server.o

clean:
	rm -rf *.o all client server
