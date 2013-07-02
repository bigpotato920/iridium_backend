#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "iridium.h"
#include "network.h"

char* msg_send[MAX_IRIDIUM_NUM];


int init_msg_send()
{
	
	int i;
	for (i = 0; i < MAX_IRIDIUM_NUM; i++) {
		msg_send[i] = (char*)calloc(sizeof(char), MAX_PACKET_SIZE);
		if (msg_send[i] == NULL) {
			free_msg_send();
			return -1;
		}
	}
	return 0;
}


void free_msg_send()
{
	int i;
	for (i = 0; i < MAX_IRIDIUM_NUM; i++) {
		free(msg_send[i]);
	}

}

/**
 * Get the payload of the message send by iridium gss
 * @param  msg_recv buffer to store the payload
 * @return          0 on success, -1 on failure
 */
int get_payload(char* msg_recv)
{

	return 0;
}
/**
 * Receive message from the iridium gss and get the payload of the message
 * @param  client_fd socket descriptor of the connected clinet
 * @param  msg_recv  buffer to store the payload of the message
 * @return           actual bytes read or -1 on error
 */
int recvfrom_iridium(int client_fd, char* msg_recv)
{
	int nread;
	char msg_tmp[SBD_MO_HEADER + SBD_MO_PAYLOAD + 1];

	nread = read(client_fd, msg_tmp, SBD_MO_HEADER + SBD_MO_PAYLOAD);
	if (nread > 0) {
		msg_tmp[nread] = '\0';
		strcpy(msg_recv, msg_tmp + SBD_MO_HEADER);
	}
	
	return nread;
}

/**
 * Receive messages send by the iridium gss	
 * @param server_fd server socket descriptor
 * @param forward_ip the destination ip address
 * @return 	0 on success or -1 on failure
 */
int receive_iridium_msgs(int server_fd) 
{
	int client_fd;
	int current_sn = 0;

	int current_count[MAX_IRIDIUM_NUM] = {0};
	int total_count[MAX_IRIDIUM_NUM] = {0};
	unsigned long previous_ip[MAX_IRIDIUM_NUM] = {0};
	unsigned long current_ip[MAX_IRIDIUM_NUM] = {0};

	int nread = 0;
	char msg_recv[SBD_MO_PAYLOAD + 1];
	iridium_msg* m_msg = NULL;
	int i;

	while (1) {
		client_fd = accept_tcp_client(server_fd);
		printf("client_fd = %d\n", client_fd);
		if (client_fd < 0)
			continue;
		if ((nread = recvfrom_iridium(client_fd, msg_recv)) > 0) {
			m_msg = (iridium_msg*)msg_recv;
		
			current_sn = m_msg->sn;
			current_ip[current_sn] = m_msg->ip;
			total_count[current_sn] = m_msg->count;

			if (previous_ip[current_sn] == 0 || previous_ip[current_sn] == current_ip[current_sn]) {
				if (previous_ip[current_sn] == 0) {
					previous_ip[current_sn] = current_ip[current_sn];
					current_count[current_sn] = 0;
				}
				resemble_iridium_msgs(current_sn, m_msg->index, m_msg->msg);
				current_count[current_sn] = current_count[current_sn] + 1;

				for (i = 0; i < MAX_IRIDIUM_NUM; i++) {
					if ((total_count[i] > 0 ) && (current_count[i] == total_count[i])) {
						previous_ip[i] = 0;
						current_count[i] = 0;
						total_count[i] = 0;
						forward_iridium_msg(current_ip[i], msg_send[i]);
					}
				}

			} else {
				perror("error occured");
				close(client_fd);
				return -1;
			}	
		} 
		else if (nread == 0) {
			printf("a clien disconnected\n");	
			close(client_fd);
		} else {
			perror("error occured while reading from iridium");
			close(client_fd);
			return -1;
		}
	}

	return 0;
}

/**
 * Resemble the slice messages into a complete message
 * @param  index the index of the slice message
 * @param  msg   slice message
 * @return       [description]
 */
void resemble_iridium_msgs(int sn, int index, char* msg)
{
	printf("index = %d, msg = %s\n", index, msg);

	unsigned int msg_len = strlen(msg);
	msg_len = msg_len <= SBD_MO_PAYLOAD - HEADER_LEN ? msg_len: SBD_MO_PAYLOAD - HEADER_LEN;
	memcpy(msg_send[sn] + index*(SBD_MO_PAYLOAD - HEADER_LEN), msg, msg_len);

}


/**
 * Forward the message to specific ip address
 * @param  ip          the destination ip address
 * @param  forward_msg message to send
 * @return             0 on success or -1 on failure
 */
int forward_iridium_msg (unsigned long ip, const char* forward_msg) 
{
	printf("ip = %lu, forward_msg = %s\n", ip, forward_msg);
	return send_to_udp_client(ip, forward_msg);
}

int main(int argc, char const *argv[])
{
	int server_fd;
	int rv;

	server_fd = create_tcp_server(TCP_SERVER_IP, TCP_SERVER_PORT);
	if (server_fd < 0) {
		exit(EXIT_FAILURE);
	}

	if ((rv = init_msg_send()) < 0) {
		perror("failed to init send message");
		exit(EXIT_FAILURE);
	}


	rv = receive_iridium_msgs(server_fd);

	close(server_fd);
	free_msg_send();
	exit(EXIT_SUCCESS);
}