#include "bankingServer.h"

typedef struct account account;
struct account {
	char name[256];
	double balance;
	bool in_session;
	int hash;
	account *next;
};

#define SIZE_OF_DB 256

account* database[SIZE_OF_DB] = {NULL};

int hash_account_name(char account_name[256]) 
{
	int i;
	int sum = 0;
	for(i = 0; i < strlen(account_name); i++) {
		sum += account_name[i];
	}
	
	return sum % SIZE_OF_DB;
}

bool insert_into_list(account* new_account)
{
	account *ptr = database[new_account->hash];
	account *prev = NULL;
	while(ptr->next) {
		//account already exists
		if(ptr->name == new_account->name) return false;

		prev = ptr;
		ptr = ptr->next;
	}

	prev->next = new_account;
}

bool insert_into_db(account *new_account)
{
	if(database[new_account->hash]) {
		return insert_into_list(new_account);
	}

	database[new_account->hash] = new_account;

	return true;
}

account* get_account(char account_name[256])
{
	account *ptr = database[hash_account_name(account_name)];
	while(ptr) {
		//found account
		if(ptr->name == account_name) {
			return ptr;
		}
	}

	//account not found
	return NULL;
}

bool create_account(char account_name[256]) 
{
	account *new_account = malloc(sizeof(account));
	strcpy(new_account->name, account_name);
	new_account-> balance = 0.0;
	new_account->in_session = false;
	new_account->hash = hash_account_name(account_name);

	return insert_into_db(new_account);
}

bool toogle_session(char account_name[256]) 
{
	account *account = get_account(account_name);

	//account does not exists 
	if(!account) return false;

	account->in_session = !account->in_session;

	return true;
}

bool deposit(char account_name[256], double amount)
{
	account *account = get_account(account_name);
	
	//account does not exist
	if(!account) return false;

	account->balance += amount;

	return true;
}

bool withdraw(char account_name[255], double amount) 
{
	account *account = get_account(account_name);

	//account does not exist 
	if(!account) return false;

	//not enough money
	if(account->balance - amount < 0) return false;

	account->balance -= amount;

	return true;
}

double query_balance(char account_name[255])
{
	account *account = get_account(account_name);

	//account does not exist 
	if(!account) return false;

	return account->balance;
}

