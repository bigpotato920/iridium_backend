#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "network.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define SBD_MO_MAX 25
#define MAX_PACKET_SIZE 1960

typedef struct
{
	int sn;
	long ip;
	int index;
	int count;
	char msg[];
} iridium_msg;

#define HEADER_LEN sizeof(iridium_msg);

int recvfrom_iridium(int client_fd, char* msg_recv)
{
	int nread;

	nread = read(client_fd, msg_recv, SBD_MO_MAX);
	if (nread > 0) {
		msg_recv[nread] = '\0';
	}

	iridium_msg* m_msg = (iridium_msg*)msg_recv;
	printf("sn = %d, ip = %ld, index = %d, count = %d ,msg = %s\n", m_msg->sn, m_msg->ip, m_msg->index, m_msg->count, m_msg->msg);

	return nread;
}


int main(int argc, char const *argv[])
{
	int server_fd;
	int client_fd;
	char msg_recv[512];

	server_fd = create_tcp_server(TCP_SERVER_IP, TCP_SERVER_PORT);
	if (server_fd < 0) {
		exit(EXIT_FAILURE);
	}

	client_fd = accept_tcp_client(server_fd);
	recvfrom_iridium(client_fd, msg_recv);

	close(server_fd);
	exit(EXIT_SUCCESS);
}