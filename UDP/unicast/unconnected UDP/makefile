all: client server

client: UDP_client.o
	cc UDP_client.o -o client

server: UDP_server.o
	cc UDP_server.o -o server

UDP_client.o: UDP_client.c
	cc -c UDP_client.c -o UDP_client.o

UDP_server.o: UDP_server.c
	cc -c UDP_server.c -o UDP_server.o

clean:
	rm -rf *.o all client server
