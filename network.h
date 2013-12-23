#ifndef NETWORK_H
#define NETWORK_H

#define UNIX_SERVER_PATH "/var/tmp/server_path"
#define IRIDIUM_TCP_SERVER_IP "12.47.179.12"
#define IRIDIUM_TCP_SERVER_PORT 10800
#define TCP_SERVER_IP "127.0.0.1"
#define TCP_SERVER_PORT 8888

#define QLEN 1

int create_unix_server(const char* path_name);
int unix_server_accept(int server_fd);
int create_tcp_server(const char* ip, int port);
int accept_tcp_client(int server_fd);
int create_tcp_client();
int connect_to_tcp_server(int client_fd, const char* ip, int port);
int send_to_udp_server(unsigned long ip, int port, const char* msg);
#endif