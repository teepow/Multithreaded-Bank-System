#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
int main(int argc, char** argv) 
{
	int sockfd;  
	struct addrinfo hints, *servinfo, *p;
	int rv;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET; // use AF_INET6 to force IPv6
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0) {
	    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
	    exit(1);
	}

for(p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype,
				                p->ai_protocol)) == -1) {
            perror("socket");
            continue;
        }

    if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            perror("connect here ");
            close(sockfd);
            continue;
        }

    break; // if we get here, we must have connected successfully
}

if (p == NULL) {
    fprintf(stderr, "failed to connect\n");
        exit(2);

}

char server_response[256];
recv(sockfd, server_response, sizeof(server_response), 0);

printf("the server says: %s\n", server_response);

close(sockfd);

freeaddrinfo(servinfo);
}
