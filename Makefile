all: server client

server: Server.c
	gcc -Wall -pedantic --std=c11 Server.c -o server
	
client: Client.c
	gcc -Wall -pedantic --std=c11 Client.c -o client -pthread