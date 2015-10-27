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
		printf("connection ok\n");
		char str[100];
		//int n = read(sockfd, str, 100);
		//printf("%s", str);
	}
	printf("1\n");
	fgets(sentence, 4096, stdin);
	len = strlen(sentence);
	sentence[len-1] = '\0';
	send(sockfd, sentence, len-1, 0);
	recv(sockfd, sentence, sizeof(sentence), 0);
	printf("%s\n", sentence);
	//int w = write(sockfd, sentence, len-1);
	//int r = read(sockfd, sentence, 100);
	//printf("%s\n", sentence);
	//Log In
	/*while(1){
		char match[100] =  "ERROR: user name wrong.";
		fgets(sentence, 4096, stdin);
		len = strlen(sentence);
		sentence[len-1] = '\0';
		int w = write(sockfd, sentence, len-1);
		int r = read(sockfd, sentence, 100);
		printf("%s\n", sentence);
		if (strcmp(sentence, match) == 0){
			continue;
		}
		break;
	}
	
	//PASS word
	while(1){
		char match[100] =  "you should input valid password";
		fgets(sentence, 4096, stdin);
		len = strlen(sentence);
		sentence[len-1] = '\0';
		int w = write(sockfd, sentence, len-1);
		int r = read(sockfd, sentence, 100);
		printf("%s\n", sentence);
		if (strcmp(sentence, match) == 0){
			continue;
		}
		break;
	}*/


	//printf("FROM SERVER: %s", sentence);

	close(sockfd);
	printf("2\n");

	return 0;
}
