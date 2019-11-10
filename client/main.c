#include <string.h> 
#include <ctype.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <signal.h>

#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#include <pthread.h>

#include "utils.h"

#define BUFF_SIZE 1024
#define NAME_BUFF_SIZE 50

#define SERVER_HOST "127.0.0.1"

int socket_fd = -1;

char name_buff[NAME_BUFF_SIZE];
char buff[BUFF_SIZE];

pthread_t thread_id;

void ask_for_termination() {
    kill(getpid(), SIGINT);
}

void close_socket() {
    if (socket_fd > 0) {
        printf("\nClosing server socket...\n");
        shutdown(socket_fd, SHUT_RDWR);
        close(socket_fd);
    }
}

void* listening_runner(void* arg) {
    char recv_buff[BUFF_SIZE];

    while (1) {
        bzero(recv_buff, BUFF_SIZE);
        int received_bytes = recv(socket_fd, &recv_buff, BUFF_SIZE, 0);
        if (received_bytes <= 0) {
            ask_for_termination();
            return NULL;
        }

        printf("%s\n", recv_buff);
    }

    return NULL;
}

/**
 * Signal handler function
 */
void signal_handler(int sig) {
    print_goodbye();
    
    close_socket();

    pthread_join(thread_id, NULL);

    exit(0);
}

/**
 * Register the signal handlers. 
 */
void init_signal_handlers() {
    struct sigaction sa;

    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    
    sigaction(SIGINT, &sa, NULL);
}

/**
 * Prompt the user for input and format it into 
 * message form: [<name>] <message>
 * 
 */
void get_named_input() {
    bzero(buff, BUFF_SIZE); 
    printf(">");

    int offset = 0;
    int name_len = strlen(name_buff);
    buff[offset++] = '[';
    memcpy(buff + offset, name_buff, name_len + offset);
    offset += name_len;
    buff[offset++] = ']';
    buff[offset++] = ' ';

    while ((buff[offset++] = getchar()) != '\n') { }

    buff[offset - 1] = '\0';
}

int main() { 
    init_signal_handlers();

    print_greeting();

    get_name(name_buff, NAME_BUFF_SIZE);

    printf("Hello, %s!\n", name_buff);

    /**
     * Create a socket which will be used to connect to the server.
     */
    socket_fd = socket(AF_INET, SOCK_STREAM, DEFAULT_FLAGS);
    if (socket_fd < 0) { 
        perror("Error while creating a socket\n");
        exit(-1); 
    }
  
    /**
     * Create and initialize a structure for holding internet address.
     */
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr)); 

    int port = get_server_port();
  
    /**
     * Address family that the socket is going to bind for. 
     * In this case IPv4.
     */
    servaddr.sin_family = AF_INET; 

    /**
     * Port which server is listening to.
     */
    servaddr.sin_port = htons(port); 

    /**
     * Convert internet address from the IPv4 format to binary form 
     * in network byte order
     */
    servaddr.sin_addr.s_addr = inet_addr(SERVER_HOST); 
  

    /**
     * Create a connection between provided socket and the specified address.
     */
    if (connect(socket_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) { 
        perror("Cannot connect to given server.\n");
        close_socket(); 
        exit(-1); 
    }

    printf("Connected to server successfully!\n");    

    /**
     * Run a thread which will listen for incoming messages and print them.
     */
    if (pthread_create(&thread_id, NULL, listening_runner, NULL) != 0) {
        perror("Error while creating a thread");
        close_socket(); 
        exit(-1);
    }

    while(true) { 
        get_named_input();

        send(socket_fd, buff, strlen(buff), 0);
    } 
} 