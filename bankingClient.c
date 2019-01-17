#include "bankingClient.h"
#include <regex.h>

/**************************************************************
 * Banking Client
 *
 * main: 
 * 	make connection to server
 *     	spawn user_input_runner and server_response threads
 *
 * user_input_runner:
 * 	read input from stdin
 * 	validate input
 * 	send message to server
 *
 * server_response_runner:
 * 	receive response from server
 * 	print response to stdout
 ***************************************************************/

int main(int argc, char** argv) 
{
	if(argc != 3) {
		fprintf(stderr, "wrong number of arguments\n");
		exit(EXIT_FAILURE);
	}

	int sockfd = connect_to_server(argv[1], argv[2]);

	if(sockfd == -1) {
		fprintf(stderr, "failed to connect to server\n");
		exit(EXIT_FAILURE);
	}

	server server_info;
	server_info.server_name = argv[1];
	server_info.port_num = argv[2];
	server_info.sockfd = sockfd;

	pthread_t input_runner_id;
	pthread_t server_runner_id;

	pthread_create(&input_runner_id, NULL, user_input_runner, &server_info);
	pthread_create(&server_runner_id, NULL, server_response_runner, &server_info);

	pthread_join(server_runner_id, NULL);
	pthread_join(input_runner_id, NULL);

	return 0;
}

/* Attempt to connect to server every 3 seconds
 *
 * @param1 server_name the domain name for the server
 * @param2 port_num the string representation of the port number
 *
 * @return int the file descriptor for the server 
 */
int connect_to_server(char server_name[256], char port_num[10])
{
	int sockfd, ret;
	struct addrinfo hints, *server_addr_info, *ptr;

	memset(&hints, 0, sizeof(hints));

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	ret = getaddrinfo(server_name, port_num, &hints, &server_addr_info);
	if(ret != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
	}

	ptr = server_addr_info;

	while(ptr) {
		sockfd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if(sockfd == -1) {
			perror("socket: ");
			ptr = ptr->ai_next;
			continue;
		}


		while(1) {
			int connect_ret = connect(sockfd, ptr->ai_addr, ptr->ai_addrlen);
			if(connect_ret != -1) break;
			sleep(3);
		}

		//made connection
		printf("connected to server\n");
		freeaddrinfo(server_addr_info);
		return sockfd;
	}

	//failed to connect
	return -1;
}

/* Thread runner to accpet user input.
 * Checks that input is valid then sends the message to the server
 *
 * @param1 arg a void pointer to a server struct containing
 * 	   the server name, port number, and file descriptor for the server
 */
void * user_input_runner(void* arg) 
{
	server *server_info  = (server*) arg;

	while(1) {
		char user_input[263] = {'\0'};
		get_user_input(user_input);

		if(!input_is_valid(user_input)) {
			printf("Invalid input. Correct usage:\n\tcreate <accountname (char) >\n\tserve <accountname (char) >"
					"\n\tdeposit <amount (double) >\n\twithdraw <amount (double) >"
					"\n\tquery\n\tend\n\tquit\n");
			continue;
		}

		if(strcmp(user_input, "quit") == 0) {
			printf("disconnecting from server\n");
			break;
		}

		send_message_to_server(user_input, server_info);

		sleep(2);
	} 

	_exit(EXIT_SUCCESS);
	return;
}

/* Read input from stdin
 *
 * @param1 user_input char pointer in which to put the input
 */
void get_user_input(char user_input[263])
{
	printf("What would you like to do: \n");
	
	char *buffer = malloc(263);
	size_t input_size = 263;
	getline(&buffer, &input_size, stdin);


	int i = 0;
	while(buffer[i] != '\0' && i < 263) {
		user_input[i] = buffer[i];
		i++;
	}

	free(buffer);

	user_input[strlen(user_input) -1 ] = '\0';
}

/* Checks that the user input is valid by the rules defined in proj description
 *
 * @param1 user_input the input read from stdin
 *
 * @return 1 if input is valid; 0 otherwise
 */
int input_is_valid(char user_input[263])
{
	regex_t regex;
	int reti;
	reti = regcomp(&regex, "(^(create|serve)[  ].+)"
			"|^(end|query|quit)$"
			"|(^(deposit|withdraw)[  ][0-9]+((\\.[0-9]+)?)|(\\.[0-9]+))$", REG_EXTENDED);
	if(reti) {
		fprintf(stderr, "Could not compile regex");
		exit(EXIT_FAILURE);
	}
	reti = regexec(&regex, user_input, 0, NULL, 0);

	regfree(&regex);

	return !reti;
}

/* Send valid input to the server
 *
 * @param1 user_input the input read from stdin
 * @param2 server_info server struct containing info on the server
 */
void send_message_to_server(char user_input[263], server *server_info)
{
	int ret = send(server_info->sockfd, user_input, 263, 0);

	if(ret <= 0) {
		fprintf(stderr, "failed to send message to server\n");
	}

	return;
}

/* Thread runner to receive responses from server and print them to stdout
 *
 * @param1 arg void pointer to server struct containing info pertaining to the server
 */
void * server_response_runner(void* arg) 
{
	int ret;
	server *server_info = (server*) arg;
	char server_response[320];

	while(1) {
		ret = recv(server_info->sockfd, server_response, sizeof(server_response), 0);
		if(ret == -1) {
			perror("receive: ");
			pthread_exit(NULL);
			return;
		}

		if(strcmp(server_response, "Server has been shutdown") == 0) {
			printf("server has disconnected from client\n");
			_exit(EXIT_SUCCESS);
		}

		printf("response from sever: %s\n", server_response);
	}

	return;
}
