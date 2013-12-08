#ifdef __cplusplus
extern "C" {
#endif
    
#include "csapp.h"
    
#ifdef __cplusplus
}
#endif

#include "global.h"

int main(int argc, char** argv){

	int listenfd, connfd, port;
	socklen_t clientlen;
	struct sockaddr_in clientaddr;
	struct hostent *hp;
	char* haddrp;
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <port>\n", argv[0]);
		exit(0);
	}
	port = atoi(argv[1]);

	listenfd = Open_listenfd(port);
	while(1) {
		clientlen = sizeof(clientaddr);
		connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);

		/* Determine the domain name and IP address of the client */
		hp = Gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, sizeof(clientaddr.sin_addr.s_addr), AF_INET);
		haddrp = inet_ntoa(clientaddr.sin_addr);
		printf("Server connected to %s (%s)\n", hp->h_name, haddrp);

		echo(connfd);
		Close(connfd);
	}
	exit(0);
}

