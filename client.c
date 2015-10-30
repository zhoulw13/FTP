#include <sys/socket.h>
#include <netinet/in.h>

#include <unistd.h>
#include <errno.h>

#include <string.h>
#include <memory.h>
#include <stdio.h>

int getAddrPort(char *sentence, char *addr);
int serverSocketInit(int port);
int clientSocketInit(int port, const char *addr);

int main(int argc, char **argv) {
	int sockfd;
	char sendMsg[8192];
	char recvMsg[8192];
	int len;
	int state = 0;
	int filePort = -1;
	char fileAddr[100] = "\0";
	int filefd;
	int connfd = 0;

	sockfd = clientSocketInit(21, "127.0.0.1");
	if (sockfd == -1){
		return -1;
	}
	
	//Other request
	while(1){
		fgets(sendMsg, 4096, stdin);
		len = strlen(sendMsg);
		sendMsg[len-1] = '\0';
		if (strcmp(sendMsg, "\0") == 0)
			continue;
		send(sockfd, sendMsg, len-1, 0);
		char pass[20] = "\0";
		strncpy(pass, sendMsg, 4);
		
		char mask[20] = "\0";
		recv(sockfd, recvMsg, sizeof(recvMsg), 0);
		printf("%s\n", recvMsg);
		strncpy(mask, recvMsg, 3);	
		
		if (strcmp(pass, "USER") == 0){
			if (strcmp(mask, "331") == 0){
				state = 1;
			}
		}else if(strcmp(pass, "PASS") == 0 && state == 1){
			if (strcmp(mask, "230") == 0){
				state = 2;
			}
		}else if (strcmp(pass, "QUIT") == 0 || strcmp(pass, "ABOR") == 0){
			if (strcmp(mask, "221") == 0){ //QUIT
				break;
			}
		}
		if (state >= 2){ //user logined
			if(strcmp(pass, "PASV") == 0){
				if (strcmp(mask, "227") == 0){
					filePort = getAddrPort(recvMsg+4, fileAddr);
					connfd = clientSocketInit(filePort, fileAddr);
					state = 4;
				}
			}else if(strcmp(pass, "PORT") == 0){
				if (strcmp(mask, "200") == 0){
					filePort = getAddrPort(sendMsg+5, fileAddr);
					filefd = serverSocketInit(filePort);
					if (filefd != -1){
						state = 3;
					}
				}
			}else if(strcmp(pass, "RETR") == 0){
				if (strcmp(mask, "150") == 0){
					if(state == 3){
						connfd = accept(filefd, NULL, NULL);	
					}
					char text[8192]="\0";
					FILE *fp = fopen(sendMsg+5,"wb+");
					int length = 0;
					while(1){
						length = recv(connfd, text, sizeof(text), 0);
						if (strcmp(text, "file end zhoulw copyright") == 0)
							break;
						fwrite(text, sizeof(char), length, fp);
					}
					fclose(fp);
					memset(recvMsg, 0, sizeof(recvMsg));
					recv(sockfd, recvMsg, sizeof(recvMsg), 0);
					printf("%s\n", recvMsg);
					if(state == 3){
						close(filefd);
						connfd = 0;
					}
					state = 2;
				}
			}else if(strcmp(pass, "STOR") == 0){
				if (strcmp(mask, "300") == 0){
					char filename[100];
					strcpy(filename, sendMsg+5);
					char buf[1000], text[8192] = "\0";
					FILE *fp = fopen(filename, "rb");
					if (fp == NULL){
						send(sockfd, "550 file dose not exist\r\n", 100, 0);
					}else{
						if (state == 3){
							connfd = accept(filefd, NULL, NULL);
						}
						char msg[] = "150 Opening BINARY mode data connection for ";
						strcat(msg, filename);
						strcat(msg, "\r\n");
						send(sockfd, msg, 100, 0);
						
						while(fread(text, sizeof(char), 8192, fp) > 0){
							if (send(connfd, text, sizeof(text), 0) < 0 ) {
								printf("failed\n");
								break;
							}
							memset(text, 0, sizeof(text));
						}
						send(connfd, "file end zhoulw copyright", 100, 0);
						fclose(fp);
						
						memset(recvMsg, 0, sizeof(recvMsg));
						recv(sockfd, recvMsg, sizeof(recvMsg), 0);
						printf("%s\n", recvMsg);
						
						if(state == 3){
							close(filefd);
							filefd = 0;
						}
						state = 2;
					}
				}
			}
		}
		memset(sendMsg, 0, sizeof(sendMsg));
		memset(recvMsg, 0, sizeof(recvMsg));
	}

	close(sockfd);
	if(filefd != 0){
		close(filefd);
	}

	return 0;
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

int clientSocketInit(int port, const char *ip){
	int sockfd;
	struct sockaddr_in addr;
	if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		printf("Error socket(): %s(%d)\n", strerror(errno), errno);
		return -1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = port;
	if (inet_pton(AF_INET, ip, &addr.sin_addr) < 0) {
		printf("Error inet_pton(): %s(%d)\n", strerror(errno), errno);
		return -1;
	}

	if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		printf("Error connect(): %s(%d)\n", strerror(errno), errno);
		return -1;
	}else{
		printf("connect to server successfully\n");
	}
	
	return sockfd;
}

int serverSocketInit(int port){
	int listenfd;
	struct sockaddr_in addr;
	if ((listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		printf("Error socket(): %s(%d)\n", strerror(errno), errno);
		return -1;
	}
	printf("SERVER socket() OK\n");

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = port;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(listenfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		printf("Error bind(): %s(%d)\n", strerror(errno), errno);
		return -1;
	}
	printf("SERVER bind() OK\n");

	if (listen(listenfd, 10) == -1) {
		printf("Error listen(): %s(%d)\n", strerror(errno), errno);
		return -1;
	}
	printf("SERVER listen() OK\n");
	return listenfd;
}
