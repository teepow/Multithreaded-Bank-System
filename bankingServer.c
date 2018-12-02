#include "bankingClient.h"
#include "bankingServer.h"
#include <limits.h>

int main(int argc, char** argv) 
{
	if(argc != 2) {
		fprintf(stderr, "wrong number of arguments\n");
		exit(EXIT_FAILURE);
	}

	int server_sockfd = bind_to_socket(argv[1]);

	pthread_t request_runner_id;

	pthread_create(&request_runner_id, NULL, request_acceptance_runner, &server_sockfd);

	pthread_join(request_runner_id, NULL);

	return 0;
}


int bind_to_socket(char port_num[10]) 
{
 	int server_sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if(server_sockfd == -1) {
		perror("socket: ");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(atoi(port_num));
	server_address.sin_addr.s_addr = INADDR_ANY;

	int ret = bind(server_sockfd, (struct sockaddr*) &server_address, sizeof(server_address));
	if(ret == -1) {
		perror("bind: ");
		exit(EXIT_FAILURE);
	}	

	return server_sockfd;
}

void * request_acceptance_runner(void* arg)
{
	int server_sockfd = *((int*) arg);
	
	listen(server_sockfd, INT_MAX);

	int client_sockfd;
	while(1) {
		printf("accepting\n");
		client_sockfd = accept(server_sockfd, NULL, NULL);
		if(client_sockfd >= 0) {
			printf("accepted\n");
			pthread_t service_runner_id;
			pthread_create(&service_runner_id, NULL, client_service_runner, &client_sockfd);
		}	
		sleep(2);
	}
}

void * client_service_runner(void* arg)
{
	int client_sockfd = *((int*) arg);
	char client_message[300];

	while(1) {
		recv(client_sockfd, client_message, sizeof(client_message), 0);
		db_command db_command = get_
	}
}

db_command get_db_command(char client_message[300])
{
	char command_string[9];
	parse_command_from_message(client_message, command_string);

	char *commands[] = {"create", "serve", "deposit", "withdraw", "query"};

	int i;
	for(i = 0; i < 5; i++) {
		if(strcmp(command_string, commands[i]) == 0) return i;
	}

	return -1;
}

void parse_command_from_message(char client_message[300], char command_string[9])
{
	//query command does not need parsing
	if(strcmp(client_message, "query") == 0) return "query";

	int i = 0;
	while(client_message[i] != ' ') {
		command_string[i] = client_message[i];
		i++;
	}

	command_string[i] = '\0';

	return;
}











