#include "bankingServer.h"
#include <limits.h>
#include <signal.h>
#include <sys/time.h>
#include <fcntl.h>
#include <semaphore.h>

/******************************* GLOBALS **************************************/
sem_t semaphore;

pthread_mutex_t mutex;

/* this is set to true by handle_sigint(); used to notify threads of a SIGINT */
bool sig_int_called;

/* this is set by client_service_runner(); 
 * used to notify request_acceptance_runner() of when services have shut down
 */
bool services_shut_down;

/* this is used to keep track of the number of services running;
 * if there are no services running when a SIGINT is signalled,
 * then request_acceptance_runner() just needs to pthread_exit();
 * otherwise it must wait for its services to shutdown
 */
int num_clients = 0;

/******************************************************************************/

int main(int argc, char** argv) 
{
	if(argc != 2) {
		fprintf(stderr, "wrong number of arguments\n");
		exit(EXIT_FAILURE);
	}
	
	sem_init(&semaphore, 0, 1);

	signal(SIGALRM, sigalrm_print);

	struct itimerval it_val;

	it_val.it_value.tv_sec = 15;
	it_val.it_value.tv_usec = 15000;
	it_val.it_interval = it_val.it_value;

	int ret = setitimer(ITIMER_REAL, &it_val, NULL);
	if(ret == -1) {
		perror("error with settititmer\n");
	}

	
	int server_sockfd = bind_to_socket(argv[1]);

	sig_t sig_ret = signal(SIGINT, handle_sigint);
	if(sig_ret == SIG_ERR) {
		perror("sigint: ");
	}

	pthread_t request_runner_id;
	pthread_create(&request_runner_id, NULL, request_acceptance_runner, &server_sockfd);
	pthread_join(request_runner_id, NULL);

	free_db();

	return 0;
}

/* prints database every 15 seconds via SIGALRM */
void sigalrm_print()
{
	sem_wait(&semaphore);
	print_db();
	sem_post(&semaphore);
}

/* bind server to socket
 * 
 * @param1 port_num port number entered by user
 *
 * @return socket file descriptor for the bound server
 */
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

/* handles the SIGINT interupt;
 * stops timer and sets sig_int_called to true to alert other threads of SIGINT
 */
void handle_sigint()
{
	struct itimerval it_val;

	it_val.it_value.tv_sec = 0;
	it_val.it_value.tv_usec = 0;
	it_val.it_interval = it_val.it_value;

	int ret = setitimer(ITIMER_REAL, &it_val, NULL);
	if(ret == -1) {
		perror("error with settititmer\n");
	}

	sig_int_called = true;
}

/* thread runner to accept requests from client 
 * listens on server socket for requests
 * creates a new client_service_runner for each incoming request
 *
 * @param1 arg void pointer to server_sockfd
 */
void * request_acceptance_runner(void* arg)
{
	int server_sockfd = *((int*) arg);
	//if we don't do this, the loop hangs at accept() and we never know if services_shut_down has been set
	make_calls_to_socket_nonblocking(server_sockfd);
	
	//list of pthread_t ids in structs with next nodes
	service_runner_id_node *service_id_list = NULL;

	listen(server_sockfd, INT_MAX);

	int client_sockfd;
	while(1) {
		//if sigint was called and the service runner has finished, join child threads, close server socket,  and exit own thread
		if(services_shut_down) {
			join_threads(service_id_list);
			free_service_ids(service_id_list);
			pthread_exit(NULL);
			return;
		}

		//if sigint was called and there are no services running, exit the thread
		if(sig_int_called && num_clients == 0) {
			pthread_exit(NULL);
			return;
		}

		int ret_accept = accept(server_sockfd, NULL, NULL);

		//if we have a request, set client_sockfd to that file descriptor, print that connection was made
		//run a service thread, add thread id to list, and set services_running to true.
		if(ret_accept >= 0) {
			client_sockfd = ret_accept;

			printf("accepted connection from client #%d\n", client_sockfd);

			pthread_t service_runner_id;
			pthread_create(&service_runner_id, NULL, client_service_runner, &client_sockfd);
			
			add_id_to_list(service_runner_id, &service_id_list);
			
			pthread_mutex_lock(&mutex);
			num_clients++;
			pthread_mutex_unlock(&mutex);
		}	
	}
}

/* makes calls to a socket non-blocking so they do not stall loops
 *
 * @param1 fd the file descriptor for the socket 
 */
void make_calls_to_socket_nonblocking(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

/* join service threads 
 *
 * @param1 pointer to the list of service nodes
 */
void join_threads(service_runner_id_node *service_id_list)
{
	while(service_id_list) {
		pthread_join(service_id_list->id, NULL);
		service_id_list = service_id_list->next;
	}
}

/* free the list of service_runner_nodes
 *
 * @param1 service_id_list pointer to the list of id nodes 
 */
void free_service_ids(service_runner_id_node *service_id_list)
{
	service_runner_id_node *ptr = service_id_list;
	while(ptr) {
		service_runner_id_node *tmp = ptr->next;
		free(ptr);
		ptr = NULL;
		ptr = tmp;
	}
}


/* add a new id to the list of id nodes
 *
 * @param1 id the id of the thread to be added
 * @param2 service_id_list double pointer to list of id nodes 
 */
void add_id_to_list(pthread_t id, service_runner_id_node **service_id_list) 
{
	service_runner_id_node *new_node = malloc(sizeof(service_runner_id_node));
	new_node->id = id;
	new_node->next = NULL;

	if(!(*service_id_list)) {
		*service_id_list = new_node;
		return;
	}

	service_runner_id_node *ptr = *service_id_list;
	while(ptr->next) {
		ptr = ptr->next;
	}

	ptr->next = new_node;
}

/* thread runner to handle client sessions
 * accepts commands from client and executes them on database
 * sends error and success messages to client after execution
 *
 * @param1 void pointer to client_sockfd
 */
void * client_service_runner(void* arg)
{
	int client_sockfd = *((int*) arg);
	//if we don't do this, the loop hangs and we never know if sig_int_called has been set
	make_calls_to_socket_nonblocking(client_sockfd);

	char client_message[300];
	char account_name[256] = {'\0'};

	bool active_session = false;
	char active_session_account_name[256] = {'\0'};

	while(1) {
		//if sigint was called tell client, close the client socket, and exit the thread
		if(sig_int_called) {
			shutdown_service(client_sockfd);
			return;
		}

		double amount;
		double balance;
		int status = 0;

		//get message from client
		int recv_ret = recv(client_sockfd, client_message, sizeof(client_message), 0);

		//no data received
		if(recv_ret == -1) continue;

		//client disconnected
		if(recv_ret == 0 || strcmp(client_message, "quit") == 0) {
			printf("disconnecting from client #%d\n", client_sockfd);

			//if client was in session, end it
			if(active_session)
				exec_db_command(END, account_name, 0);

			//close the connection
			disconnect_from_client(client_sockfd);

			pthread_mutex_lock(&mutex);
			num_clients--;
			pthread_mutex_unlock(&mutex);

			break;
		}

		//parse command from message (CREATE, SERVE, DEPOSIT, WITHDRAW, QUERY, or END)
		db_command command = get_db_command(client_message);

		//get account name if needed
		//otherwise use the account name of the active session
		if(account_name_needed(command))
			get_account_name(client_message, account_name);
		else 
			strcpy(account_name, active_session_account_name);

		//get amount if needed
		if(amount_needed(command))
			amount = get_amount(client_message);

		//if session needs to be active to execute command and is not active, set error code
		if(active_session_needed(command) && !active_session) 
			status = -6;

		//if session needs to be inactive to execute command and is active, set error code
		if(!active_session_needed(command) && active_session)
			status = -7;

		//query returns a value, so we execute it separately if the session is in the correct state
		if(status == 0 && command == QUERY) {
			sem_wait(&semaphore);
			balance = query_balance(account_name);
			sem_post(&semaphore);
		}

		//if the session is in the correct state and the command is not a query, execute the command
		if(status == 0 && command != QUERY){
			sem_wait(&semaphore);
			status = exec_db_command(command, account_name, amount);
			sem_post(&semaphore);
		}

		//if there was an error, alert the client 
		if(status != 0) 
			send_error_to_client(status, client_sockfd);
		
		//if execution was successful, send message to client
		if(status == 0)
			send_message_to_client(command, balance,  client_sockfd);

		//handle successful execution of the end command
		if(status == 0 && command == END) {
			active_session = false;
			active_session_account_name[0] = '\0';
		}

		//handle successful execution of the serve command
		if(status == 0 && command == SERVE) {
			active_session = true;
			strcpy(active_session_account_name, account_name);
		}
	}
}

/* send shutdown message to client; close socket; set services_shut_down to alert request_acceptance_runner()
 * that services have shutdown; exit thread
 *
 * @param1 client_sockfd the file descriptor for the client socket to shutdown
 */
void shutdown_service(int client_sockfd)
{
	char message[25] =  "Server has been shutdown";
	send(client_sockfd, message, sizeof(message), 0);

	disconnect_from_client(client_sockfd);

	services_shut_down = true;
	pthread_exit(NULL);
}

/* print that server is disconnecting from client and disconnect from client
 *
 * @param1 client_sockfd file descriptor for client
 */
void disconnect_from_client(int client_sockfd)
{
	printf("disconnecting from client #%d\n", client_sockfd);

	int close_ret = close(client_sockfd);
	if(close_ret == -1) {
		perror("close service: ");
	}
}

/* get command (CREATE, SERVE, DEPOSIT, WITHDRAW, QUERY, or END) from incoming client message
 *
 * @param1 client_message the message from the client 
 *
 * @return int representing db_command enum value
 */
db_command get_db_command(char client_message[300])
{
	char command_string[9] = {'\0'};
	parse_command_from_message(client_message, command_string);

	char *commands[] = {"create", "serve", "deposit", "withdraw", "query", "end"};

	int i;
	for(i = 0; i < 6; i++) {
		if(strcmp(command_string, commands[i]) == 0) return i;
	}

	return -1;
}

/* helpe for get_db_command; parses the command from the client message
 *
 * param1 client_message the message from the client
 * param2 command_string pointer in which to put parsed command
 */
void parse_command_from_message(char client_message[300], char command_string[9])
{
	//query and end commands do not need parsing
	if(strcmp(client_message, "query") == 0 || strcmp(client_message, "end") == 0)  {
		strcpy(command_string, client_message);
		return;
	}

	int i = 0;
	while(client_message[i] != ' ') {
		command_string[i] = client_message[i];
		i++;
	}

	command_string[i] = '\0';

	return;
}

/* determine if the client message has an account name that needs to be parsed
 *
 * param1 command the command sent by client
 *
 * @return true if name needs to be parsed; false otherwise
 */
bool account_name_needed(db_command command)
{
	return command == CREATE || command == SERVE;
}

/* parse account name from client message
 *
 * param1 client message the message from the client
 * param2 account_name pointer in which to put parsed name
 */
void get_account_name(char client_message[300], char account_name[256])
{
	int i = 0;
	while(client_message[i] != ' ')	i++;
	
	i++;

	int k = 0;
	while(client_message[i] != '\0') {
		account_name[k] = client_message[i];
		i++;
		k++;
	}

	account_name[k] = '\0';

	return;
}

/* determine if the client message has an amount that needs to be parsed
 *
 * param1 command the command sent by the client
 *
 * @return true if amount needs to be parsed; false otherwise
 */
bool amount_needed(db_command command) 
{
	return command == DEPOSIT || command == WITHDRAW;
}

/* parse amount from client message
 *
 * param1 client_message the message from the client
 *
 * @return double value of amount in client message
 */
double get_amount(char client_message[300])
{
	//317 is max chars in dbl on ilabs
	char dbl_string[317];

	int i = 0;
	while(client_message[i] != ' ') i++;
	
	i++;

	int k = 0;
	while(client_message[i] != '\0') {
		dbl_string[k] = client_message[i];
		i++;
		k++;
	}		

	dbl_string[k] = '\0';

	return atof(dbl_string);	
}

/* determine if command needs an active session to be executed
 *
 * @param1 command the command sent by the client
 *
 * @return true if session must be active; false if session must not be active
 */
bool active_session_needed(db_command command) 
{
	return command == DEPOSIT || command == WITHDRAW || command == QUERY || command == END;
}

/* execute CREATE, SERVE, DEPOSIT, WITHDRAW, and END commands on the database
 * (Note that query commands are executed separately)
 *
 * @param1 command the command sent by the client (required)
 * @param2 name of account to execute command for (optional)
 * @param3 amount value of deposit or withdrawl (optional)
 *
 *@return status 
 * 	-1 account already exists
 *	-2 account does not exist 
 *	-3 account already in session 
 *	-4 account not in session 
 *	-5 insufficient funds 
 *       0 success 
*/
int exec_db_command(db_command command, char account_name[256], double amount)
{
	int status = 0;
	switch(command) {
		case CREATE:
			status = create_account(account_name);
			break;
		case SERVE:
			status = start_session(account_name);
			break;
		case DEPOSIT:
			status = deposit(account_name, amount);
			break;
		case WITHDRAW:
			status = withdraw(account_name, amount);
			break;
		case QUERY:
			status = query_balance(account_name);
			break;
		case END: 
			status = end_session(account_name);
			break;
		default:
			break;
	}
	
	return status;
}

/* send an error message to the client
 *
 * @param1 status holds the error code of the falied command
 * @param2 client_sockfd file descriptor of client socket
 */
void send_error_to_client(int status, int client_sockfd)
{
	char message[40] = {'\0'};
	switch(status) {
		case -1:
			strcpy(message, "ERROR: Account already exists\n");
			break;
		case -2:
			strcpy(message, "ERROR: Account does not exist\n");
			break;
		case -3:
			strcpy(message, "ERROR: Account already in session\n");
			break;
		case -4:
			strcpy(message, "ERROR: Account not in session\n");
			break;
		case -5:
			strcpy(message, "ERROR: Insufficient funds\n");
			break;
		case -6:
			strcpy(message, "ERROR: Must be in active session\n");
			break;
		case -7:
			strcpy(message, "ERROR: Must not be in active session\n");
			break;
		default:
			break;
	}

	send(client_sockfd, message, sizeof(message), 0);
	
	return;
}

/* send success message to the client
 *
 * @param1 command the command that was executed (required)
 * @param2 balance the current balance returned by QUERY execution (optional)
 * param3 client_sockfd file descriptor of client socket (required)
 */
void send_message_to_client(db_command command, double balance, int client_sockfd)
{
	char message[40] = {'\0'};
	switch(command) {
		case CREATE:
			strcpy(message, "SUCCESS: New account created\n");
			break;
		case SERVE:
			strcpy(message, "SUCCESS: New session started\n");
			break;
		case DEPOSIT:
			strcpy(message, "SUCCESS: Deposit made\n");
			break;
		case WITHDRAW:
			strcpy(message, "SUCCESS: Withdrawl made\n");
			break;
		case QUERY:
			//357 because a double can be 317 chars
			snprintf(message, 357, "Your current balance is: %f\n", balance);
			break;
		case END: 
			strcpy(message, "SUCCESS: Session ended\n");
			break;
		default:
			break;
	}

	send(client_sockfd, message, sizeof(message), 0);
}
