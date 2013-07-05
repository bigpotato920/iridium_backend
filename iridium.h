#ifndef IRIDIUM_H
#define IRIDIUM_H

#include <stdint.h>

#define SBD_MO_PAYLOAD 25

#define MAX_PACKET_SIZE 1960
#define MAX_IRIDIUM_NUM 3
#define SBD_MO_HEADER 51

typedef struct
{
	int sn; // sequential number of the iridium terminal
	unsigned long ip; // destination ip address
	int index; // index of the specific slice
	int count; // totoal count of the slices
	char msg[]; // payload of the message

} iridium_mo_msg;


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

#define MO_HEADER_LEN sizeof(iridium_mo_msg)
#define MT_HEADER_LEN sizeof(iridium_mt_msg)


int init_msg_send();
void free_msg_send();

int get_payload(char* msg_recv);
int recvfrom_iridium(int client_fd, char* msg_recv);
int receive_iridium_msgs(int server_fd) ;
void resemble_iridium_msgs(int sn, int index, char* msg);
int forward_iridium_msg (unsigned long ip, const char* forward_msg);
int forward_server_command(int unix_server_fd, int tcp_client_fd);
int sendto_iridium(int tcp_client_fd, const char* imei, const char* command);
#endif