#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "network.h"

#define SLICE_LEN 100  
#define SBD_MO_HEADER_LEN 51
#define MAX_PACKET_SIZE 1960
#define MAX_IRIDIUM_NUM 3

#define CONFIG_FILENAME "backend_config"

char unix_server_path[256];
char iridium_tcp_server_ip[256];
int iridium_tcp_server_port;
char local_tcp_server_ip[256];
int local_tcp_server_port;

typedef struct
{
	int sn; // sequential number of the iridium terminal
	unsigned long ip; // destination ip address
    int port; // port num
	int index; // index of the specific slice
	int count; // totoal count of the slices

} slice_header;

#define SLICE_HEADER_LEN sizeof(slice_header)
#define SLICE_PAYLOAD_LEN (SLICE_LEN-SLICE_HEADER_LEN)

typedef struct 
{
	slice_header header;
	char payload[];
} slice;

typedef struct 
{
	slice_header header;
	int current_count;
	int payload_len;
	char payload[MAX_PACKET_SIZE];
} mo_msg;

typedef struct
{
	uint8_t pro_num;
	uint16_t msg_len;
	uint8_t header_iei;
	uint16_t header_len;
	char client_msg_id[4];
	char imei[15];
	uint16_t flag;
	uint8_t payload_iei;
	uint16_t payload_len;
	char payload[];

} iridium_mt_msg;


typedef struct 
{
	char imei[15];
	char command[];

} server_command;

#define MO_HEADER_LEN sizeof(slice)
#define SBD_MT_HEADER_LEN sizeof(iridium_mt_msg)



int init();

int get_payload(char* msg_recv);
int get_slice(int client_fd, slice* slice);
int receive_iridium_msgs(int server_fd);
int resemble_iridium_msgs(slice* slice, int payload_len);
int forward_iridium_msg (int sn);
int forward_server_command(int unix_server_fd, int tcp_client_fd);
int sendto_iridium(int tcp_client_fd, const char* imei, const char* command);
int read_config(const char *filename);
mo_msg mo_msgs[MAX_IRIDIUM_NUM];

int init()
{
	int i;
	for (i = 0; i < MAX_IRIDIUM_NUM; i++) {
		mo_msgs[i].current_count = 0;
		mo_msgs[i].payload_len = 0;
	}

	read_config(CONFIG_FILENAME);
	return 0;
}

/**
 * Read configuration from specific file
 * 
 * @param filename name of the configuration file
 * @return 0 on success or -1 on failure
 */
int read_config(const char *filename)
{
	FILE *file;
	int rv;
	char key[100] = {0};
	char value[100] = {0};

	file = fopen(filename, "r");

	while ((rv = fscanf(file, "%s %s", key, value)) > 0)
	{
	    if (!(strcmp(key, "UNIX_SERVER_PATH"))) {
	        strcpy(unix_server_path, value);
	    } else if (!(strcmp(key, "IRIDIUM_TCP_SERVER_IP"))) {
	        strcpy(iridium_tcp_server_ip, value);
	    } else if (!(strcmp(key, "IRIDIUM_TCP_SERVER_PORT"))) {
	        iridium_tcp_server_port = atoi(value);
	    } else if (!(strcmp(key, "LOCAL_TCP_SERVER_IP"))) {
	        strcpy(local_tcp_server_ip, value);
	    } else if (!(strcmp(key, "LOCAL_TCP_SERVER_PORT"))) {
	        local_tcp_server_port = atoi(value);
	    } 
	}
	fclose(file);

	return 0;
}
/**
 * Retrieve a single message from the iridium gss
 * @param  client_fd socket descriptor of the connected clinet
 * @param  msg_recv  buffer to store the payload of the message
 * @return           actual bytes read or -1 on error
 */
int get_slice(int client_fd, slice* slice)
{
	int nread = -1;
	int buffer_len = SBD_MO_HEADER_LEN + SLICE_LEN;
	char buffer[buffer_len];
	memset(buffer, 0, buffer_len);
	nread = read(client_fd, buffer, buffer_len);
	memcpy(slice, buffer+SBD_MO_HEADER_LEN, SLICE_LEN);

	return nread - SBD_MO_HEADER_LEN - SLICE_HEADER_LEN;
}

/**
 * Receive messages from the iridium gss send by different iridium terminal
 * @param server_fd server socket descriptor
 * @param forward_ip the destination ip address
 * @return 	-1 on failure, 0 on client disconnect, n(>0) bytes read
 */
int receive_iridium_msgs(int client_fd) 
{
	int payload_len = 0;
	int finish = 0;
	slice *slice = malloc(SLICE_LEN);


	payload_len = get_slice(client_fd, slice);
	finish = resemble_iridium_msgs(slice, payload_len);

	if (finish)
		forward_iridium_msg(slice->header.sn);

	free(slice);
	return 0;
}

/**
 * Resemble the slice messages into a complete message
 * @param  index the index of the slice message
 * @param  msg   slice message
 * @return       [description]
 */
int resemble_iridium_msgs(slice *slice, int payload_len)
{
	printf("index = %d, payload_len = %d, msg = %s\n", slice->header.index, 
		payload_len, slice->payload);
	int sn = slice->header.sn;
	int index = slice->header.index;
	mo_msgs[sn].header = slice->header;
	int i;
	mo_msgs[sn].payload_len += payload_len;
	mo_msgs[sn].current_count += 1;
	memcpy(mo_msgs[sn].payload + index*(SLICE_PAYLOAD_LEN), slice->payload,
		payload_len);

	//resemble complete
	if (mo_msgs[sn].current_count == slice->header.count) {
		mo_msgs[sn].current_count = 0;
		mo_msgs[sn].payload[mo_msgs[sn].payload_len] = '\0';
		printf("total payload len = %d\n", mo_msgs[sn].payload_len);
		

		for (i = 0; i < mo_msgs[sn].payload_len; i++)
			printf("%c", mo_msgs[sn].payload[i]);
		printf("\n");
		mo_msgs[sn].payload_len = 0;

		return 1;
	}

	return 0;
}


/**
 * Forward the message to specific ip address
 * @param  ip          the destination ip address
 * @param  forward_msg message to send
 * @return             0 on success or -1 on failure
 */
int forward_iridium_msg (int sn) 
{
	unsigned long ip = mo_msgs[sn].header.ip;
	int port = mo_msgs[sn].header.port;
	printf("ip = %lu, port = %d, len = %d, forward_msg = %s\n", 
		ip, mo_msgs[sn].header.port, mo_msgs[sn].payload_len, mo_msgs[sn].payload);
	return send_to_udp_server(ip, port, mo_msgs[sn].payload);
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

	rv = connect_to_tcp_server(tcp_client_fd, iridium_tcp_server_ip, iridium_tcp_server_port);
	if (rv < 0) {
		perror("Failed to connect to the iridium gss server");
		return -1;
	}
	rv = sendto_iridium(tcp_client_fd, m_command.imei, m_command.command);

	close(unix_client_fd);

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
	int total_len = SBD_MT_HEADER_LEN + command_len;
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
	int test_fd;
    fd_set readfds;
    fd_set testfds;

	if ((rv = init()) < 0) {
		perror("failed to init send message");
		exit(EXIT_FAILURE);
	}


	tcp_server_fd = create_tcp_server(local_tcp_server_ip, local_tcp_server_port);
	if (tcp_server_fd < 0) {
		perror("Failed to create tcp server");
		exit(EXIT_FAILURE);
	}

	tcp_client_fd = create_tcp_client();
	if (tcp_client_fd < 0) {
		perror("Failed to create tcp client");
		exit(EXIT_FAILURE);
	}


	unix_server_fd = create_unix_server(unix_server_path);
	if (unix_server_fd < 0) {
		perror("Failed to create unix server");
		exit(EXIT_FAILURE);
	}

    FD_ZERO(&readfds);
    FD_SET(tcp_server_fd, &readfds);
    FD_SET(unix_server_fd, &readfds);   

    while (1) {

        testfds = readfds;
        rv = select(FD_SETSIZE, &testfds, (fd_set *)NULL, (fd_set *)NULL, (struct timeval *)NULL);
        switch (rv)
        {
    
        case -1:
            perror("select");
            break;
        default:

        	for (test_fd = 0; test_fd < FD_SETSIZE; test_fd++) {
        		if (FD_ISSET(test_fd, &testfds)) {
        			if (test_fd == tcp_server_fd) {
            			temp_client_fd = accept_tcp_client(tcp_server_fd);
            			printf("a client connected\n");
            			FD_SET(temp_client_fd, &readfds);
            	
            		}
            		else if (test_fd == unix_server_fd) {
            			rv = forward_server_command(unix_server_fd, tcp_client_fd);
            		} 

            		else {
            			rv = receive_iridium_msgs(test_fd);
            			if (rv <= 0) {
            				printf("a client disconnect\n");
            				FD_CLR(test_fd, &readfds);
            				close(test_fd);
            			}
            		}
        		}
	
        	}

        }
    }
	
	close(tcp_server_fd);
	close(unix_server_fd);
	close(tcp_client_fd);

	exit(EXIT_SUCCESS);
}
