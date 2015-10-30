#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/ioctl.h> 
#include <netinet/in.h>
#include <netdb.h>  
#include <net/if.h>  
#include <arpa/inet.h> 

#include <unistd.h>
#include <errno.h>

#include <ctype.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

//client state: 
//0:user name needed, 1:password needed, 2:user has login
//3:port assigned, 4:passive mode
//5:data trasmiting
int clientState[100] = {0};

char clientAddr[100][50];
int clientPort[100]={0};
int fileSocket[100]={0};
int fileConnfd[100]={0};
void handleClientRequest(int client, char *buf);
int getAddrPort(char *buf, char *addr);
char *get_ip();
int serverAddrPort(char *addr, char *sentence);
int serverSocketInit(int port);
int clientSocketInit(int port, const char *addr);

int main(int argc, char **argv) {
	int listenfd, connfd;
	struct sockaddr_in clientaddr;
	int addrlen;
	int nbytes;
	int i, j;
	int fdmax;
	char pattern[10][10] = {"USER", "PASS", "PORT", "PASV", "RETR", "STOR", "SYST", "TYPE", "QUIT", "ABOR"};
	
	listenfd = serverSocketInit(6789);
	if (listenfd == -1){
		return -1;
	}
	
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
						if(clientState[i] != 5){ //ignore command when data transmiting
							handleClientRequest(i, sentence);
						}
					}
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
	}else if(strcmp(pass, "QUIT") == 0 || strcmp(pass, "ABOR") == 0){
		send(clientfd, "221 Good bye\r\n", 100, 0);
		return;
	}else if(strcmp(pass, "PORT") == 0){
		clientPort[clientfd] = getAddrPort(sentence, clientAddr[clientfd]);
		send(clientfd, "200 port received\r\n", 100, 0);
		clientState[clientfd] = 3;
		return;
	}else if(strcmp(pass, "PASV") == 0){
		memset(sentence, 0, sizeof(sentence));
		int port = serverAddrPort(get_ip(), sentence);
		if (fileSocket[clientfd] != 0){ // close the old connection
			close(fileSocket[clientfd]);
		}
		fileSocket[clientfd] = serverSocketInit(port); //create a new port and listen to it
		if (fileSocket[clientfd] == -1){ //PASV error
			send(clientfd, "400 PASV error\r\n", 100, 0);
		}else{
			strcat(sentence, "\r\n");
			char temp[10000] = "227 ";
			strcat(temp, sentence);
			clientState[clientfd] = 4;
			send(clientfd, temp, strlen(temp), 0);
		}
		return;
	}else if(strcmp(pass, "RETR") == 0){
		if(clientState[clientfd] != 3 && clientState[clientfd] !=4){
			send(clientfd, "425 no TCP connection established\r\n", 100, 0);
			return;
		}
		char filename[100];
		strcpy(filename, sentence+5);
		if (clientState[clientfd] == 3){
			fileConnfd[clientfd] = clientSocketInit(clientPort[clientfd], clientAddr[clientfd]);
		}
		else if(clientState[clientfd] == 4){
			fileConnfd[clientfd] = accept(fileSocket[clientfd], NULL, NULL);
		}
		if (fileConnfd[clientfd] == -1){
			send(clientfd, "425 can't connect to assigned address\r\n", 100, 0);
			return;
		}
		FILE *fp = fopen(filename, "r");
		if (fp == NULL){
			send(clientfd, "451 can't open assigned file\r\n", 100, 0);
			return;
		}
		char buf[1000], text[8192] = "\0";
		while (!feof(fp)){
            if(fgets(buf,1000,fp)!=NULL){
            	if(strlen(buf) + strlen(text) <= sizeof(text))
			        strcat(text, buf);
			    else{
			    	send(clientfd, "450 out of memory\r\n", 100, 0);
			    	fclose(fp);
			    	return;
			    }
			}
	    }
	    fclose(fp);
	    printf("%s\n", text);
	    send(clientfd, "226 text file loaded\r\n", 100, 0);
	    
	    //sending data
	    clientState[clientfd] = 5;
    	send(fileConnfd[clientfd], text, strlen(text), 0);
    	clientState[clientfd] = 2;
    	
    	//data send ok
    	close(fileSocket[clientfd]);
    	fileSocket[clientfd] = 0;
		return;
	}else if(strcmp(pass, "STOR") == 0){
		return;
	}
	send(clientfd, "500 REQUEST not found\r\n", 100, 0);
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

char *get_ip()  
{  
    int fd, num;  
    struct ifreq ifq[16];  
    struct ifconf ifc;  
    int i;  
    char *ips, *tmp_ip;  
    char *delim = ",";  
    int val;  
      
    fd = socket(AF_INET, SOCK_DGRAM, 0);  
    if(fd < 0){  
        fprintf(stderr, "socket failed\n");  
        return NULL;  
    }  
    ifc.ifc_len = sizeof(ifq);  
    ifc.ifc_buf = (caddr_t)ifq;  
    if(ioctl(fd, SIOCGIFCONF, (char *)&ifc)){  
        fprintf(stderr, "ioctl failed\n");  
        return NULL;  
    }  
    num = ifc.ifc_len / sizeof(struct ifreq);  
    if(ioctl(fd, SIOCGIFADDR, (char *)&ifq[num-1])){  
        fprintf(stderr, "ioctl failed\n");  
        return NULL;  
    }  
    close(fd);  
      
    val = 0;  
    for(i=0; i<num; i++){  
        tmp_ip = inet_ntoa(((struct sockaddr_in*)(&ifq[i].ifr_addr))-> sin_addr);  
        if(strcmp(tmp_ip, "127.0.0.1") != 0){  
            val++;  
        }  
    }  
      
    ips = (char *)malloc(val * 16 * sizeof(char));  
    if(ips == NULL){  
        fprintf(stderr, "malloc failed\n");  
        return NULL;  
    }  
    memset(ips, 0, val * 16 * sizeof(char));  
    val = 0;  
    for(i=0; i<num; i++){  
        tmp_ip = inet_ntoa(((struct sockaddr_in*)(&ifq[i].ifr_addr))-> sin_addr);  
        if(strcmp(tmp_ip, "127.0.0.1") != 0){  
            if(val > 0){  
                strcat(ips, delim);  
            }  
            strcat(ips, tmp_ip);  
            val ++;  
        }  
    }  
      
    return ips;  
}

int serverAddrPort(char *addr, char *sentence){
	srand(time(NULL));
	int port = rand() % (65535 - 1 + 20000) + 20000;
	int i, j;
	char str[100];
	strcpy(sentence, addr);
	int len = strlen(sentence);
	sentence[len] = ',';
	sentence[len+1] = '\0';
	len = strlen(sentence);
	sprintf(str, "%d", port/256);
	strcpy(sentence+len, str);
	len = strlen(sentence);
	sentence[len] = ',';
	sentence[len+1] = '\0';
	len = strlen(sentence);
	sprintf(str, "%d", port%256);
	strcpy(sentence+len, str);
	for (i = 0; i < len; i++){
		if(sentence[i] == '.')
			sentence[i] = ',';
	}
	
	return port;
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
		printf("connect to client data port successfully\n");
	}
	
	return sockfd;
}
