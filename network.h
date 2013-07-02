#ifndef NETWORK_H
#define NETWORK_H

#define TCP_SERVER_IP "127.0.0.1"
#define TCP_SERVER_PORT 8888
#define UDP_SERVER_PORT 6666

int create_tcp_server(const char* ip, int port);
int accept_tcp_client(int server_fd);
int send_to_udp_client(unsigned long ip, const char* msg);
#endif