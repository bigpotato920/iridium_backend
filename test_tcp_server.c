#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define SERVER_PORT 8888
#define SERVER_IP "127.0.0.1"
#define SBD_MO_MAX 25

typedef struct
{
   int sn;
   unsigned long ip;
   int index;
   int count;
   char msg[];
} iridium_msg;

#define HEADER_LEN sizeof(iridium_msg)

int main(int argc, char**argv)
{
   int server_fd;
   struct sockaddr_in server_addr;

   iridium_msg* m_msg = (iridium_msg*)malloc(sizeof(iridium_msg) + SBD_MO_MAX - HEADER_LEN);
   const char* msgs[] = {"hello wor", "ld I come", " from Chi", "na"};

   if (argc < 2) {
      printf("Usage:%s index\n", argv[0]);
      exit(EXIT_FAILURE);
   }

   int sn = atoi(argv[1]);
   int index = atoi(argv[2]);

   server_fd = socket(AF_INET,SOCK_STREAM,0);

   memset(&server_addr, 0, sizeof(server_addr));
   server_addr.sin_family = AF_INET;
   server_addr.sin_addr.s_addr=inet_addr(SERVER_IP);
   server_addr.sin_port=htons(SERVER_PORT);

   connect(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));

   m_msg->sn = sn;
   m_msg->ip = 2676598976;
   m_msg->index = index;
   m_msg->count = 4;
   
   memcpy(m_msg->msg, msgs[index], SBD_MO_MAX - HEADER_LEN);
   write(server_fd, m_msg, sizeof(iridium_msg) + SBD_MO_MAX - HEADER_LEN);
   close(server_fd);

   free(m_msg);
   exit(EXIT_SUCCESS);
}