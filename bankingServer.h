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

/* enums */
typedef enum _bool{false, true} bool;
typedef enum _db_command{CREATE, SERVE, DEPOSIT, WITHDRAW, QUERY} db_command;

/* server functions */
int bind_to_socket(char port_num[10]);
void * request_acceptance_runner(void* arg);
void * client_service_runner(void* arg);
