all: client server

client: client.cc
	q++ -o client client.cc 

server: server.cc
	q++ -o server server.cc

clean:
	rm -f server client