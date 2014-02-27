#ifndef NETWORK_H
#define NETWORK_H


#define QLEN 1

int create_unix_server(const char* path_name);
int unix_server_accept(int server_fd);
int create_tcp_server(const char* ip, int port);
int accept_tcp_client(int server_fd);
int create_tcp_client();
int connect_to_tcp_server(int client_fd, const char* ip, int port);
int send_to_udp_server(unsigned long ip, int port, const char* msg);
#endif