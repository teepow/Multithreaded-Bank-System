/*************************************************************************
 * This file handles all of the database operations for the banking server 
                                                               
   Error codes:
 	-1 account already exists
 	-2 account does not exist 
 	-3 account already in session 
 	-4 account not in session 
 	-5 insufficient funds 
         0 success 
 ***************************************************************************/
#include "database.h"

typedef struct account account;
struct account {
	char name[256];
	double balance;
	int in_session;
	int hash;
	account *next;
};

#define SIZE_OF_DB 256

account* database[SIZE_OF_DB] = {NULL};

/*
 * hash an account name 
 *
 * @param1 account_name name of the account 
 *
 * return int the hash value
 */
int hash_account_name(char account_name[256]) 
{
	int i;
	int sum = 0;
	for(i = 0; i < strlen(account_name); i++) {
		sum += account_name[i];
	}
	
	return sum % SIZE_OF_DB;
}

/* insert into linked list of database;
 * helper function for insert_into_db to resolve collisions
 *
 * @param1 new_account the account to be inserted
 *
 * @return -1 if account already exists 
 *          0 if successful
 */
int insert_into_list(account* new_account)
{
	account *ptr = database[new_account->hash];
	account *prev = NULL;
	while(ptr) {
		//account already exists
		if(strcmp(ptr->name, new_account->name) == 0) return -1;

		prev = ptr;
		ptr = ptr->next;
	}

	prev->next = new_account;

	return 0;
}

/* insert account into database 
 *
 * @param1 the account to be inserted 
 *
 * @return -1 if account already exists 
 *          0 if successful 
 */
int insert_into_db(account *new_account)
{
	if(database[new_account->hash]) {
		//returns -1 if account exists; 0 if successful
		return insert_into_list(new_account);
	}

	database[new_account->hash] = new_account;

	return 0;
}

/* retrieve account from database 
 *
 * @param1 account_name name of account 
 *
 * @return pointer to account if found 
 *         NULL if account not found
 */
account* get_account(char account_name[256])
{
	account *ptr = database[hash_account_name(account_name)];
	while(ptr) {
		//found account
		if(strcmp(ptr->name, account_name) == 0) {
			return ptr;
		}
	}

	//account not found
	return NULL;
}

/* create a new account 
 *
 * @param1 account_name name of account 
 *
 * @return -1 if account already exists 
 *          0 if successful
 */
int create_account(char account_name[256]) 
{
	account *new_account = malloc(sizeof(account));
	strcpy(new_account->name, account_name);
	new_account-> balance = 0.0;
	new_account->in_session = 0;
	new_account->hash = hash_account_name(account_name);

	return insert_into_db(new_account);
}

/* start new session 
 *
 * @param1 account_name name of account
 *
 * @return -2 if account does not exist 
 *         -3 if account is already in session 
 *          0 if successful
 */
int start_session(char account_name[256]) 
{
	account *account = get_account(account_name);

	//account does not exist 
	if(!account) return -2;

	//already in session
	if(account->in_session) return -3;

	account->in_session = 1;

	return 0;
}

/* end session 
 *
 * @param1 account_name name of account
 *
 * @return -2 if account does not exist 
 *         -4 if account is already not in session 
 *          0 if successful
 */
int end_session(char account_name[256])
{
	account *account = get_account(account_name);

	//account does not exist 
	if(!account) return -2;

	//account not in session
	if(!account->in_session) return -4;

	account->in_session = 0;

	return 0;
}

/* deposit into account 
 *
 * @param1 account_name name of account 
 * @param2 amount double value of amount to be deposited
 *
 * @return -2 if account does not exist 
 *          0 if successful
 */
int deposit(char account_name[256], double amount)
{
	account *account = get_account(account_name);
	
	//account does not exist
	if(!account) return -2;

	account->balance += amount;

	return 0;
}

/* withdraw from account 
 *
 * @param1 account_name name of account 
 * @param2 amount double value of amount to be withdrawn 
 *
 * @return -2 if account does not exist 
 *         -5 if insufficient funds 
 *          0 if successful 
 */
int withdraw(char account_name[255], double amount) 
{
	account *account = get_account(account_name);

	//account does not exist 
	if(!account) return -2;

	//not enough money
	if(account->balance - amount < 0) return -5;

	account->balance -= amount;

	return 0;
}

/* retrieve account balance 
 *
 * @param1 account_name name of account
 *
 * @return -2 if account does not exist 
 *          account balance if successful
 */
double query_balance(char account_name[255])
{
	account *account = get_account(account_name);

	//account does not exist 
	if(!account) return -2;

	return account->balance;
}

/* print account information for single account */
void print_account_info(account* account) 
{
	char* in_session = (account->in_session) ? "IN SERVICE" : "";

	printf("%s\t%f\t%s\n\n", account->name, account->balance, in_session);
}

/* print the database */
void print_db()
{
	int i;
	for(i = 0; i < SIZE_OF_DB; i++) {
		account *ptr = database[i];
		while(ptr) {
			print_account_info(ptr);
			ptr = ptr->next;
		}
	}
}

/* free the database */
void free_db()
{
	int i;
	for(i = 0; i < SIZE_OF_DB; i++) {
		account *ptr = database[i];
		while(ptr) {
			account* tmp = ptr->next;
			free(ptr);
			ptr = NULL;
			ptr = tmp;
		}
	}
}
