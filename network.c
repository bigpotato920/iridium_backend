#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>

#include "network.h"

/**
 * Create the tcp server on specific ip address and port
 * @param  ip   ip address		
 * @param  port port num	
 * @return      if success return tcp server socket descriptor or -1 on failure
 */
int create_tcp_server(const char* ip, int port) 
{
	int server_fd;
	struct sockaddr_in server_addr;

	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("create socket");
		return -1;
	}

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ip);
	server_addr.sin_port = htons(port);

	if (bind(server_fd, (struct sockaddr*)&server_addr,
			sizeof(server_addr)) < 0) {
		perror("socket bind");
		return -1;
	}

	if (listen(server_fd, 1) < 0) {
		perror("socket listen");
		return -1;
	}

	return server_fd;
}

/**
 * Accept the client try to connect to the server
 * @param  server_fd Server socket descriptor
 * @return           valid socket decriptor on success or -1 on failure
 */
int accept_tcp_client(int server_fd)
{
	int client_fd;

	if ((client_fd = accept(server_fd, 0, 0)) < 0) {
		perror("socket accept");
		return -1;
	}
	return client_fd;
}


/**
 * Send msg to specific udp client
 * @param  ip  ip address
 * @param  msg message to be send
 * @return     0 on success
 */
int send_to_udp_client(unsigned long ip, const char* msg) 
{

	int sock_fd;
	struct sockaddr_in server_addr;
	socklen_t sock_len;
	if ((sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("socket error");
		return -1;
	}

	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(UDP_SERVER_PORT);
	server_addr.sin_addr.s_addr = ip;
	sock_len = sizeof(struct sockaddr_in);

	sendto(sock_fd, msg, strlen(msg), 0, (struct sockaddr*)&server_addr, sock_len);
	return 0;
}