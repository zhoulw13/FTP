#include <sys/socket.h>
#include <netinet/in.h>

#include <unistd.h>
#include <errno.h>

#include <string.h>
#include <memory.h>
#include <stdio.h>

int getAddrPort(char *sentence, char *addr);
int serverSocketInit(int port);
int clientSocketInit(int port, char *addr);

int main(int argc, char **argv) {
	int sockfd;
	char sendMsg[8192];
	char recvMsg[8192];
	int len;
	int state = 0;
	int filePort = -1;
	char fileAddr[100] = "\0";
	int filefd;

	sockfd = clientSocketInit(6789, "127.0.0.1");
	if (sockfd == -1){
		return -1;
	}
	
	//Log In
	while(1){
		fgets(sendMsg, 4096, stdin);
		len = strlen(sendMsg);
		sendMsg[len-1] = '\0';
		send(sockfd, sendMsg, len-1, 0);	
		recv(sockfd, recvMsg, sizeof(recvMsg), 0);
		char pass[20] = "\0";
		strncpy(pass, recvMsg, 3);
		printf("%s\n", recvMsg);
		memset(sendMsg, 0, sizeof(sendMsg));
		memset(recvMsg, 0, sizeof(recvMsg));
		if (strcmp(pass, "331") == 0){
			break;
		}
	}
	
	//PASS word
	while(1){
		fgets(sendMsg, 4096, stdin);
		len = strlen(sendMsg);
		sendMsg[len-1] = '\0';
		send(sockfd, sendMsg, len-1, 0);	
		recv(sockfd, recvMsg, sizeof(recvMsg), 0);
		char pass[20] = "\0";
		strncpy(pass, recvMsg, 3);
		printf("%s\n", recvMsg);
		memset(sendMsg, 0, sizeof(sendMsg));
		memset(recvMsg, 0, sizeof(recvMsg));
		if (strcmp(pass, "230") == 0){
			break;
		}
	}
	
	//Other request
	while(1){
		fgets(sendMsg, 4096, stdin);
		len = strlen(sendMsg);
		sendMsg[len-1] = '\0';
		send(sockfd, sendMsg, len-1, 0);
		char pass[20] = "\0";
		strncpy(pass, sendMsg, 4);
		char mask[20] = "\0";
		recv(sockfd, recvMsg, sizeof(recvMsg), 0);
		strncpy(mask, recvMsg, 3);
		
		if (strcmp(pass, "QUIT") == 0 || strcmp(pass, "ABOR") == 0){
			if (strcmp(mask, "221") == 0){ //QUIT
				break;
			}
		}else if(strcmp(pass, "PASV") == 0){
			if (strcmp(mask, "227") == 0){
				filePort = getAddrPort(recvMsg+4, fileAddr);
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
			if (state == 4){
				filefd = clientSocketInit(filePort, fileAddr);
			}
		}else{
			
		}
		printf("%s\n", recvMsg);
		memset(sendMsg, 0, sizeof(sendMsg));
		memset(recvMsg, 0, sizeof(recvMsg));
	}

	close(sockfd);

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

int clientSocketInit(int port, char *ip){
	int sockfd;
	struct sockaddr_in addr;
	if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		printf("Error socket(): %s(%d)\n", strerror(errno), errno);
		return -1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = port;
	if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0) {
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
