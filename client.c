#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

#include <unistd.h>
#include <errno.h>

#include <string.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

void pasvIPanalysis(char *sentence, char *str);
void sentencefilter(char *sentence);
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
	
	//greeting
	recv(sockfd, recvMsg, sizeof(recvMsg), 0);
	printf("%s\n", recvMsg);
	memset(recvMsg, 0, sizeof(recvMsg));
	
	while(1){
		fgets(sendMsg, 4096, stdin);
		len = strlen(sendMsg);
		sendMsg[len-1] = '\0';
		if (strcmp(sendMsg, "\0") == 0)
			continue;
		strcat(sendMsg, "\r\n");
		
		char pass[20] = "\0";
		strncpy(pass, sendMsg, 4);
		char mask[20] = "\0";
		
		if (strcmp(pass, "STOR") != 0){
			send(sockfd, sendMsg, len+1, 0);
			recv(sockfd, recvMsg, sizeof(recvMsg), 0);
			printf("%s\n", recvMsg);
			strncpy(mask, recvMsg, 3);	
		}
		
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
					char str[100];
					pasvIPanalysis(recvMsg, str);
					filePort = getAddrPort(str, fileAddr);
					connfd = clientSocketInit(filePort, fileAddr);
					state = 4;
				}
			}else if(strcmp(pass, "PORT") == 0){
				if (strcmp(mask, "200") == 0){
					int len = strlen(sendMsg);
					sendMsg[len-2] = '\0';
					filePort = getAddrPort(sendMsg+5, fileAddr);
					filefd = serverSocketInit(filePort);
					//connfd = accept(filefd, NULL, NULL);
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
					char filename[100];
					strcpy(filename, sendMsg+5);
					int flen = strlen(filename);
					filename[flen-2] = '\0';
					FILE *fp = fopen(filename,"wb+");
					int length = 0;
					char strlength[10];
					while(1){
						length = recv(connfd, text, sizeof(text), 0);
						
						if(length == 0){
							break;
						}
						fwrite(text, sizeof(char), length, fp);
						//fwrite(text, sizeof(char), length, fp);
					}
					fclose(fp);
					
					close(connfd);
					if(state == 3){
						close(filefd);
						connfd = 0;
					}
					
					memset(recvMsg, 0, sizeof(recvMsg));
					recv(sockfd, recvMsg, sizeof(recvMsg), 0);
					printf("%s\n", recvMsg);
					
					state = 2;
				}
			}else if(strcmp(pass, "STOR") == 0){
				char filename[100];
				strcpy(filename, sendMsg+5);
				int flen = strlen(filename);
				filename[flen-2] = '\0';
				char text[8192] = "\0";
				FILE *fp = fopen(filename, "rb");
				if (fp == NULL){
					printf("550 file dose not exist\r\n");
				}else{
					send(sockfd, sendMsg, len+1, 0);
					recv(sockfd, recvMsg, sizeof(recvMsg), 0);
					printf("%s\n", recvMsg);
					if (state == 3){
						connfd = accept(filefd, NULL, NULL);
					}
					strncpy(mask, recvMsg, 3);
					if (strcmp(mask, "150") == 0){
						int length = 0;
						char strlength[10];
						while(!feof(fp)){
							length = fread(text, sizeof(char), sizeof(text), fp);
							if (send(connfd, text, length, 0) < 0 ) {
								printf("failed\n");
								break;
							}
							memset(text, 0, sizeof(text));
						}
						fclose(fp);
						close(connfd);
						connfd = 0;
						if(state == 3){
							close(filefd);
							filefd = 0;
						}
						memset(recvMsg, 0, sizeof(recvMsg));
						recv(sockfd, recvMsg, sizeof(recvMsg), 0);
						printf("%s\n", recvMsg);
										
						state = 2;
					}
				}
			}else if(strcmp(pass, "LIST") == 0){
				// just print
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

void pasvIPanalysis(char *sentence, char *str){
	int i, len = strlen(sentence);
	int start, end;
	for(i = 0; i < len; i++){
		if (sentence[i] == '(')
			start = i+1;
		else if (sentence[i] == ')')
			end = i;
	}
	strncpy(str, sentence+start, end-start);
	return;
}

void sentencefilter(char *sentence){
	int i, j, length = strlen(sentence);
	char re[8192];
	for (i=0, j=0; i<length; i++){
		if (sentence[i] == '\r' || sentence[i] == '\n')
			continue;
		re[j++] = sentence[i];
	}
	re[j] = '\0';
	strcpy(sentence, re);
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
	addr.sin_port = htons(port);
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
	addr.sin_port = htons(port);
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
