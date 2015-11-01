server: server.c 
	gcc server.c -o server -Wall -lpthread
clean: 
	rm -rf *.o ~* server
