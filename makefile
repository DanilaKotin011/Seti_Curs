all:
	g++ server.cpp -o server
	g++ client.cpp -lpthread -o  client
