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

void *clientThread(void *);

int main(int argc, char **argv) {
	int listenfd, connfd;
	struct sockaddr_in addr;
	char sentence[8192];
	int p;
	int len;
	int exit[10] = {0};
	int thread_num = 0;
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
	struct sockaddr_in clientaddr;
	int fdmax = listenfd;
	int addrlen;
	int nbytes;
	int i, j;
	while(1){
		memcpy(&read_fds, &fds, sizeof(fds));
		if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1){
			printf("Error select(): %s(%d)\n", strerror(errno), errno);
			return 1;
		}
		//printf("SERVER select() OK\n");
		for (i = 0; i <= fdmax; i++){
			//printf("%d, %d, %d\n", fdmax, i, listenfd);
			if (FD_ISSET(i, &read_fds)){
				//printf(" h\n");
				if (i == listenfd){
					addrlen = sizeof(clientaddr);
					if ((connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &addrlen)) == -1){
						printf("Error accept(): %s(%d)\n", strerror(errno), errno);
						continue;
					}else{
						printf("SERVER accept() OK\n");
						FD_SET(connfd, &fds);
						if (connfd > fdmax)
							fdmax = connfd;
						printf("New connection\n");
					}
				}else{
					//printf("hahah\n");
					//int r = read(i, sentence, sizeof(sentence));
					if(nbytes = recv(i, sentence, sizeof(sentence), 0) <= 0){
					}else{
						printf("%s\n", sentence);
						send(i, "received", 100, 0);
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
}

