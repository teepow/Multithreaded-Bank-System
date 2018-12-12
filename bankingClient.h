#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>

/* structs */
typedef struct server server;
struct server {
	char* server_name;
	char* port_num;
	int sockfd;
};

/* enums */
typedef enum _bool{false, true} bool;

/* client functions */
int connect_to_server(char server_name[256], char port_num[10]);
void * user_input_runner(void* arg);
void get_user_input(char user_input[263]);
int input_is_valid(char user_input[263]);
void send_message_to_server(char user_input[263], server *server_info);
void * server_response_runner(void* arg);
