#ifndef IRIDIUM_H
#define IRIDIUM_H

#include <stdint.h>

#define SLICE_HEADER_LEN 16
#define SLICE_LEN 25
#define SLICE_PAYLOAD_LEN (SLICE_LEN-SLICE_HEADER_LEN)
#define SBD_MO_HEADER_LEN 51
#define MAX_PACKET_SIZE 1960
#define MAX_IRIDIUM_NUM 3


typedef struct
{
	int sn; // sequential number of the iridium terminal
	unsigned long ip; // destination ip address
	int index; // index of the specific slice
	int count; // totoal count of the slices

} slice_header;

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
#endif