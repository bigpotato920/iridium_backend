#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define MAX_LEN 1024
#define SERVER_PORT 6666

int main(int argc, char const *argv[])
{

	char msg_recv[MAX_LEN];
	int sock_fd;
	socklen_t sock_len;
	struct sockaddr_in server;
	struct sockaddr_in client;
	int nread;

	if ((sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		printf("socket error\n");
		exit(EXIT_FAILURE);
	}
	bzero(&server, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(SERVER_PORT);
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	sock_len = sizeof(struct sockaddr_in);

	if ((bind(sock_fd, (struct sockaddr *)&server, sock_len)) < 0) {
		perror("bind");
		exit(EXIT_FAILURE);
	}
	while(1) {
		nread = recvfrom(sock_fd, msg_recv, MAX_LEN, 0, 
			(struct sockaddr *)&client, &sock_len);
		if (nread > 0) {
			msg_recv[nread] = '\0';
			printf("nread = %d, %s\n", nread, msg_recv);

		}
	}

	
	return 0;
}