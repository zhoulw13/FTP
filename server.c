#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <netinet/in.h>

#include <unistd.h>
#include <errno.h>

#include <ctype.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

int clientState[100] = {0};
char clientAddr[100][50];
int clientPort[100]={0};
void handleClientRequest(int client, char *buf);
int getAddrPort(char *buf, char *addr);

int main(int argc, char **argv) {
	int listenfd, connfd;
	struct sockaddr_in addr, clientaddr;
	int addrlen;
	int nbytes;
	int i, j;
	int fdmax;
	char pattern[10][10] = {"USER", "PASS", "PORT", "PASV", "RETR", "STOR", "SYST", "TYPE", "QUIT", "ABOR"};

	if ((listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		printf("Error socket(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}
	printf("SERVER socket() OK\n");

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = 6789;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(listenfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		printf("Error bind(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}
	printf("SERVER bind() OK\n");

	if (listen(listenfd, 10) == -1) {
		printf("Error listen(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}
	printf("SERVER listen() OK\n");
	
	fd_set fds, read_fds;
	FD_ZERO(&fds);
	FD_ZERO(&read_fds);
	FD_SET(listenfd, &fds);
	fdmax = listenfd;
	while(1){
		memcpy(&read_fds, &fds, sizeof(fds));
		if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1){
			printf("Error select(): %s(%d)\n", strerror(errno), errno);
			return 1;
		}
		for (i = 0; i <= fdmax; i++){
			if (FD_ISSET(i, &read_fds)){
				if (i == listenfd){
					addrlen = sizeof(clientaddr);
					if ((connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &addrlen)) == -1){
						printf("Error accept(): %s(%d)\n", strerror(errno), errno);
						continue;
					}else{
						printf("SERVER accept() OK\n");
						FD_SET(connfd, &fds);
						clientAddr[connfd][0] = '\0';
						if (connfd > fdmax)
							fdmax = connfd;
						printf("New connection\n");
					}
				}else{
					char sentence[8192] = "\0";
					if(nbytes = recv(i, sentence, sizeof(sentence), 0) <= 0){
						if (nbytes == 0)
							printf("socket hang up\n");
						else
							printf("Error recv(): %s(%d)\n", strerror(errno), errno);
						close(i);
						clientState[i] = 0;
						clientAddr[i][0] = '\0';
						FD_CLR(i, &fds);
					}else{
						
						//send(i, "received", 100, 0);
						handleClientRequest(i, sentence);
					}
					//int w = write(i, sentence, 100);
					//send(i, "hah", nbytes, 0);
					/*if (nbytes = recv(i, sentence, sizeof(sentence), 0) <= 0){
						if (nbytes == 0)
							printf("socket hang up\n");
						else
							printf("Error recv(): %s(%d)\n", strerror(errno), errno);
						printf("SERVER receive()\n");
						close(i);
						// remove connection i
						FD_CLR(i, &fds);
					}else{
						for (j = 0; j <= fdmax; j++){
							if (FD_ISSET(j, &fds)){
								if (j != listenfd && j != i){
									if (send(j, sentence, nbytes, 0) == -1)
										printf("Error send(): %s(%d)\n", strerror(errno), errno);
								}
							}
						}
					}*/
				}
			}
		}	
	}

	close(listenfd);
	return 0;
}


void handleClientRequest(int clientfd, char *sentence){
	printf("%s\n", sentence);
	printf("%d\n", clientState[clientfd]);
	if (clientState[clientfd] == 0){
		//USER anonymous
		if (strcmp(sentence, "USER anonymous") == 0){
			send(clientfd, "331 Guest login ok, send your complete e-mail address as the password.\r\n", 100, 0);
			clientState[clientfd] = 1;
		}else{
			send(clientfd, "ERROR: user name wrong\r\n", 100, 0);
		}
		return;
	}else if(clientState[clientfd] == 1){
		//PASS **@**
		char pass[20] = "\0";
		strncpy(pass, sentence, 5);
		if (strcmp(pass, "PASS ") == 0){
			send(clientfd, "230-hello word!\r\n230 welcome to zhoulw's ftp server\r\n", 100, 0);
			clientState[clientfd] = 2;
		}else{
			send(clientfd, "503 you should input valid password\r\n", 100, 0);
		}
		return;
	}
	
	char pass[20] = "\0";
	strncpy(pass, sentence, 4);
	if (strcmp(pass, "SYST") == 0){
		send(clientfd, "215 UNIX Type: L8\r\n", 100, 0);
		return;
	}else if(strcmp(pass, "TYPE") == 0 && strcmp(sentence, "TYPE I") == 0){
		send(clientfd, "200 Type set to I.\r\n", 100, 0);
		return;
	}else if(strcmp(pass, "QUIT") == 0){
		send(clientfd, "221 Good bye\r\n", 100, 0);
	}else if(strcmp(pass, "PORT") == 0){
		clientPort[clientfd] = getAddrPort(sentence, clientAddr[clientfd]);
	}else if(strcmp(pass, "PASV") == 0){
		
	}else if(strcmp(pass, "RETR") == 0){
		
	}else if(strcmp(pass, "STOR") == 0){
		
	}
	send(clientfd, "received", 100, 0);
	return;
}

int getAddrPort(char *sentence, char *addr){
	int i, j, k;
	int len = strlen(sentence);
	char port[2][10];
	for (i = len-1, j= 0, k=len-1; i >= 0 && j < 2; i--){
		if (sentence[i] == ','){
			strncpy(port[j], sentence+i+1, k-i);
			j++;
			k = i-1;
		}
	}
	strncpy(addr, sentence, i+1);
	for (; i>=0; i--){
		if (sentence[i] == ','){
			addr[i] = '.';
		}
	}
	return atoi(port[0]) + 256*atoi(port[1]);
}

