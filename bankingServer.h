#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <limits.h>
#include "database.h"

/* node for linked list of service threads */
typedef struct service_runner_id_node service_runner_id_node;
struct service_runner_id_node {
	pthread_t id; 			//thread id
	service_runner_id_node *next; 	//next node
};

/* enums */
typedef enum _db_command{CREATE, SERVE, DEPOSIT, WITHDRAW, QUERY, END} db_command;
typedef enum _bool{false, true} bool;

/* server functions */
int bind_to_socket(char port_num[10]);
void * request_acceptance_runner(void* arg);
void * client_service_runner(void* arg);
db_command get_db_command(char client_message[300]);
void parse_command_from_message(char client_message[300], char command_string[9]);
bool account_name_needed(db_command command);
void get_account_name(char client_message[300], char account_name[256]);
bool amount_needed(db_command command);
double get_amount(char client_message[300]);
bool active_session_needed(db_command command);
int exec_db_command(db_command command, char account_name[256], double amount);
void send_error_to_client(int status, int client_sockfd);
void send_message_to_client(db_command command, double balance, int client_sockfd);
void sigalrm_print();
void add_id_to_list(pthread_t id, service_runner_id_node **service_id_list);
void join_threads(service_runner_id_node *service_id_list);
void free_service_ids(service_runner_id_node *service_id_list);
void handle_sigint();
void make_calls_to_socket_nonblocking(int fd);
void shutdown_service(int client_sockfd);
void disconnect_from_client(int client_sockfd);
