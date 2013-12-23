#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>

#define SERVER_PORT 8888
//#define SERVER_IP "10.103.240.52"
#define SERVER_IP "222.128.13.159"
#define SBD_MO_MAX 30


typedef struct 
{
    char header[51];
    char payload[];
} sbd_mo_msg;


typedef struct
{
    int sn;
    long ip;
    int port;
    int index;
    int count;
    char msg[];
} iridium_msg;



int main(int argc, char**argv)
{
    int server_fd;
    struct sockaddr_in server_addr;
    int result;
    
    int index = atoi(argv[1]);
    const char* msgs[] = {"hello worl", "d I come f", "rom China"};

    server_fd = socket(AF_INET,SOCK_STREAM,0);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr=inet_addr(SERVER_IP);
    server_addr.sin_port=htons(SERVER_PORT);

    result = connect(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (result < 0) {
        printf("failed to connect to remote server, errno = %d\n", errno);
        return 1;
    }

    iridium_msg *m_msg = (iridium_msg*)malloc(30);

    m_msg->sn = 0;
    m_msg->ip = 1688381632;
    m_msg->port = 8000;
    m_msg->index = index;
    m_msg->count = 3;
    memcpy(m_msg->msg, msgs[index], 10);

    sbd_mo_msg *sbd_msg = (sbd_mo_msg*)malloc(81);

    memset(sbd_msg->header, 0, 51);
    memcpy(sbd_msg->payload, m_msg, 30);

    write(server_fd, sbd_msg, 81);

    free(m_msg);
    free(sbd_msg);

    close(server_fd);

    exit(EXIT_SUCCESS);
}
