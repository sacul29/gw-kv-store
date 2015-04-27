#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>
#include "../lib/marshal/marshal.h"
/****************************************
 Author: Joel Klein, Katie Stasaski, Lucas Chaufournier, Tim Wood
 with a little help from
 http://beej.us/guide/bgnet/
 ****************************************/
//Returns the number of bytes to wait for

char * marshal_msg(char * cmd, char * key, char * value)
{
	char * temp;
	int num_bytes;

	struct operation* marshal_msg = malloc(sizeof(struct operation));
	if (cmd[0] == 's') {
		marshal_msg -> method_type = SET;
		num_bytes = strlen(value);
		marshal_msg -> value = malloc(sizeof(num_bytes));
		marshal_msg -> value = value;
		marshal_msg -> value_length = num_bytes;
	} else {
		marshal_msg -> method_type = GET;
	}
	marshal_msg -> key = malloc(sizeof(key));
	marshal_msg -> key = key;
	marshal_msg -> key_length = strlen(key);
	gwkv_marshal_client(marshal_msg, &temp);

	return temp;
}

struct operation* demarshal_msg(int sockfd)
{
	char curr_char;
	int cont = 1;
	int i;
	char* msg = malloc(1024);
	struct operation** marshal_msg = malloc(sizeof(struct operation));
	int * status;

	//printf("reading lines\n");
	i = 0;
	for(i=0; i<3; i++) {
		while(1) {
			msg[i] = curr_char;
			i++;
			recv(sockfd, &curr_char, 1, 0);
			if(curr_char == ' ') {
				printf(" ");
				break;
			}
			printf("%c",curr_char);

		}
		cont = 1;
	}

	while(1) {
		recv(sockfd, &curr_char, 1, 0);
		if (curr_char == '\r') {
			recv(sockfd, &curr_char, 1, 0);
			if (curr_char == '\n') {
				break;
			}
		} else {
			msg[i] = curr_char;
		}
		i++;
	}
	gwkv_demarshal_client(msg, marshal_msg, status);
	if (*status == -1) {
		return 0;
	} else {
		return *marshal_msg;
	}
}

send_msg(int sockfd, char * temp)
{
	int rc;

	rc = send(sockfd,temp,strlen(temp), 0);
	if(rc < 0) {
		perror("ERROR on send");
		exit(-1);
	}
	return;
}

read_msg(int sockfd)
{
	int bytes_received;
	char * recv_data = (char *)malloc(sizeof(char*)*1000);

	struct operation* demarshaled_msg = malloc(sizeof(struct operation));
	demarshaled_msg = demarshal_msg(sockfd);

	bytes_received = recv(sockfd, recv_data, demarshaled_msg->value_length, 0);
	printf("%s\n", recv_data);
}

int main(int argc, char ** argv)
{
	//port used to connect to socket
	char* server_port = "80";
	//ip address or host name of server http request will be sent to
	char* server_ip = "google.com";
	//socket int used to connect
	int sockfd, rc, o;
	//used to store formatted http request
	struct addrinfo hints, *server;
	char * cmd;
	char * key;
	char * value;
	char * temp;

    /* Command line args:
            -p port
            -h host name or IP
	-m message (specific page you are requesting.  For ex, when
	   requesting www.cs.gwu.edu/research, the message would be
	   research.)
    */
    while ((o = getopt (argc, argv, "p:h:c:k:v:")) != -1) {
            switch(o){
            case 'p':
                    server_port = optarg;
                    break;
            case 'h':
                    server_ip = optarg;
                    break;
            case 'c':
                    cmd = optarg;
                    break;
	case 'k':
		key = optarg;
		break;
	case 'v':
		value = optarg;
		break;
            case '?':
                    if(optopt == 'p' || optopt == 'h' ) {
                            fprintf (stderr, "Option %c requires an argument.\n", optopt);
                    }
                    else {
                            fprintf (stderr, "Unknown argument: %c.\n", optopt);
                    }
                    break;
            }
    }

    /* The hints struct is used to specify what kind of server info we are looking for */
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM; /* or SOCK_DGRAM */

    /* getaddrinfo() gives us back a server address we can connect to.
       It actually gives us a linked list of addresses, but we'll just use the first.
     */
    if (rc = getaddrinfo(server_ip, server_port, &hints, &server) != 0) {
            perror(gai_strerror(rc));
            exit(-1);
    }

	sockfd = socket(server->ai_family, server->ai_socktype, server->ai_protocol);
	if (sockfd == -1) {
		perror("ERROR opening socket");
		exit(-1);
	}
	//printf("socket created\n");
	rc = connect(sockfd, server->ai_addr, server->ai_addrlen);
	if (rc == -1) {
		perror("ERROR on connect");
		close(sockfd);
		exit(-1);
		// TODO: could use goto here for error cleanup
	}

    //create the marshalled message
    temp = marshal_msg(cmd, key, value);
	//send the marshaled message to the server
	send_msg(sockfd, temp);

	//read back get value
	if (cmd[0] == 'g') {
		read_msg(sockfd);
	}


	out:
		freeaddrinfo(server);
	close(sockfd);

	//printf("Done.\n");
	return 0;

}
