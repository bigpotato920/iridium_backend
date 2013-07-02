#ifndef IRIDIUM_H
#define IRIDIUM_H


#define SBD_MO_PAYLOAD 25

#define MAX_PACKET_SIZE 1960
#define MAX_IRIDIUM_NUM 3
#define SBD_MO_HEADER 51

typedef struct
{
	int sn;
	unsigned long ip;
	int index;
	int count;
	char msg[];

} iridium_msg;

char* msg_send[MAX_IRIDIUM_NUM];
#define HEADER_LEN sizeof(iridium_msg)

int init_msg_send();
void free_msg_send();

int get_payload(char* msg_recv);
int recvfrom_iridium(int client_fd, char* msg_recv);
int receive_iridium_msgs(int server_fd) ;
void resemble_iridium_msgs(int sn, int index, char* msg);
int forward_iridium_msg (unsigned long ip, const char* forward_msg);

#endif