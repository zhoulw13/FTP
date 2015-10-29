#include <sys/socket.h>
#include <netinet/in.h>

#include <unistd.h>
#include <errno.h>

#include <string.h>
#include <memory.h>
#include <stdio.h>

int main(int argc, char **argv) {
	int sockfd;
	struct sockaddr_in addr;
	char sentence[8192];
	int len;
	int p;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		printf("Error socket(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = 6789;
	if (inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) <= 0) {
		printf("Error inet_pton(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}

	if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		printf("Error connect(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}else{
		printf("connect to server successfully\n");
	}

	//Log In
	while(1){
		fgets(sentence, 4096, stdin);
		len = strlen(sentence);
		sentence[len-1] = '\0';
		send(sockfd, sentence, len-1, 0);
		sentence[0]='\0';
		recv(sockfd, sentence, sizeof(sentence), 0);
		char pass[20] = "\0";
		strncpy(pass, sentence, 3);
		printf("%s\n", sentence);
		sentence[0]='\0';
		if (strcmp(pass, "331") == 0){
			break;
		}
	}
	sentence[0]='\0';
	
	//PASS word
	while(1){
		fgets(sentence, 4096, stdin);
		len = strlen(sentence);
		sentence[len-1] = '\0';
		send(sockfd, sentence, len-1, 0);
		sentence[0]='\0';
		recv(sockfd, sentence, sizeof(sentence), 0);
		char pass[20] = "\0";
		strncpy(pass, sentence, 3);
		printf("%s\n", sentence);
		sentence[0]='\0';
		if (strcmp(pass, "230") == 0){
			break;
		}
	}
	
	//Other request
	while(1){
		fgets(sentence, 4096, stdin);
		len = strlen(sentence);
		sentence[len-1] = '\0';
		send(sockfd, sentence, len-1, 0);
		char pass[20] = "\0";
		strncpy(pass, sentence, 4);
		sentence[0]='\0';
		recv(sockfd, sentence, sizeof(sentence), 0);
		printf("%s\n", sentence);
		if (strcmp(pass, "QUIT") == 0){
			pass[0] = '\0';
			strncpy(pass, sentence, 3);	
			if (strcmp(pass, "221") == 0){ //QUIT
				break;
			}
		}			
		sentence[0]='\0';
	}

	close(sockfd);

	return 0;
}
