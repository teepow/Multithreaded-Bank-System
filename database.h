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

int create_account(char account_name[256]); 
int start_session(char account_name[256]); 
int deposit(char account_name[256], double amount);
int withdraw(char account_name[255], double amount);
double query_balance(char account_name[255]);
int end_session(char account_name[256]);
void print_db();
