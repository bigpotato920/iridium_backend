#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "iridium.h"
#include "network.h"

char* msg_send[MAX_IRIDIUM_NUM];
int current_count[MAX_IRIDIUM_NUM] = {0};
int total_count[MAX_IRIDIUM_NUM] = {0};
unsigned long previous_ip[MAX_IRIDIUM_NUM] = {0};
unsigned long current_ip[MAX_IRIDIUM_NUM] = {0};

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
 * Retrieve a single message from the iridium gss
 * @param  client_fd socket descriptor of the connected clinet
 * @param  msg_recv  buffer to store the payload of the message
 * @return           actual bytes read or -1 on error
 */
int recvfrom_iridium(int client_fd, char* msg_recv)
{
	int nread = -1;

	nread = read(client_fd, msg_recv, MO_HEADER_LEN + SBD_MO_PAYLOAD);
	msg_recv[nread] = '\0';
	return nread;
}

/**
 * Receive messages from the iridium gss send by different iridium terminal
 * @param server_fd server socket descriptor
 * @param forward_ip the destination ip address
 * @return 	-1 on failure, 0 on client disconnect, n(>0) bytes read
 */
int receive_iridium_msgs(int client_fd) 
{
	int current_sn = 0;
	int nread = 0;
	char msg_recv[MO_HEADER_LEN + SBD_MO_PAYLOAD];
	iridium_mo_msg* m_msg = NULL;
	int i;

	printf("client_fd = %d\n", client_fd);
	if (client_fd < 0)
		return -1;
	if ((nread = recvfrom_iridium(client_fd, msg_recv)) > 0) {
		m_msg = (iridium_mo_msg*)msg_recv;
		
		current_sn = m_msg->sn;
		printf("sn = %d\n", m_msg->sn);
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
	}
	

	return nread;
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
	msg_len = msg_len <= SBD_MO_PAYLOAD - MO_HEADER_LEN ? msg_len: SBD_MO_PAYLOAD - MO_HEADER_LEN;
	memcpy(msg_send[sn] + index*(SBD_MO_PAYLOAD - MO_HEADER_LEN), msg, msg_len);

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
	return send_to_udp_server(ip, forward_msg);
}


/**
 * Forward the command send from the server to iridium gss
 * @param  unix_server_fd unix domain socket server descriptor
 * @return                0 on success or -1 on failure
 */
int forward_server_command(int unix_server_fd, int tcp_client_fd)
{
	server_command m_command;
	int unix_client_fd;
	int nread;
	int rv;

	unix_client_fd = unix_server_accept(unix_server_fd);
	nread = read(unix_client_fd, &m_command, BUFSIZ);

	rv = connect_to_tcp_server(tcp_client_fd, IRIDIUM_TCP_SERVER_IP, IRIDIUM_TCP_SERVER_PORT);
	if (rv < 0) {
		perror("Failed to connect to the iridium gss server");
		return -1;
	}
	rv = sendto_iridium(tcp_client_fd, m_command.imei, m_command.command);

	close(unix_client_fd);
	close(tcp_client_fd);

	return rv;
}


/**
 * Send specific server command to the iridium gss
 * @param  command command to be send
 * @param  imei the imei number of the iridium data terminal
 * @return         0 on success or -1 on failure
 */
int sendto_iridium(int tcp_client_fd, const char* imei, const char* command)
{
	  
	iridium_mt_msg* msg = NULL;
	int command_len = strlen(command);
	int total_len = MT_HEADER_LEN + command_len;
	int nwrite;

	msg = (iridium_mt_msg*)malloc(total_len);
	msg->pro_num = 1;
	msg->msg_len = command_len + 27;
	msg->header_iei = 65;
	msg->header_len = 21;
	memcpy(msg->client_msg_id, "msg0", 4);
	memcpy(msg->imei, imei, 15);
	msg->flag = 0;
	msg->payload_iei = 66;
	msg->payload_len = command_len;
	memcpy(msg->payload, command, command_len);

	nwrite = write(tcp_client_fd, msg, total_len);
   
	return nwrite == total_len;
}


int main(int argc, char const *argv[])
{
	int tcp_server_fd;
	int tcp_client_fd;
	int unix_server_fd;
	int temp_client_fd;
	int rv;
	int max_fd;
    fd_set readfds;
    fd_set testfds;

	if ((rv = init_msg_send()) < 0) {
		perror("failed to init send message");
		exit(EXIT_FAILURE);
	}


	tcp_server_fd = create_tcp_server(TCP_SERVER_IP, TCP_SERVER_PORT);
	if (tcp_server_fd < 0) {
		perror("Failed to create tcp server");
		exit(EXIT_FAILURE);
	}

	tcp_client_fd = create_tcp_client();
	if (tcp_client_fd < 0) {
		perror("Failed to create tcp client");
		exit(EXIT_FAILURE);
	}


	unix_server_fd = create_unix_server(UNIX_SERVER_PATH);
	if (unix_server_fd < 0) {
		perror("Failed to create unix server");
		exit(EXIT_FAILURE);
	}

    FD_ZERO(&readfds);
    FD_SET(tcp_server_fd, &readfds);
    FD_SET(unix_server_fd, &readfds);
    max_fd = tcp_server_fd > unix_server_fd ? tcp_server_fd : unix_server_fd;    

    while (1) {

        testfds = readfds;
        rv = select(FD_SETSIZE, &testfds, (fd_set *)NULL, (fd_set *)NULL, (struct timeval *)NULL);
        switch (rv)
        {
    
        case -1:
            perror("select");
            break;
        default:

            if (FD_ISSET(tcp_server_fd, &testfds)) {
            	temp_client_fd = accept_tcp_client(tcp_server_fd);
            	FD_SET(temp_client_fd, &readfds);
            	
            }

            else if (FD_ISSET(unix_server_fd, &testfds)) {
            	rv = forward_server_command(unix_server_fd, tcp_client_fd);
            } 

            else {
            	rv = receive_iridium_msgs(temp_client_fd);
            	if (rv == 0) {
            		FD_CLR(temp_client_fd, &readfds);
            	}
            }
        }
    }
	
	close(tcp_server_fd);
	free_msg_send();
	exit(EXIT_SUCCESS);
}