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

#define BUFF_SIZE 1024
#define NAME_BUFF_SIZE 50

#define PORT 5000

#define true 1
#define DEFAULT_FLAGS 0

int socket_fd = -1;

char name_buff[50];
char buff[BUFF_SIZE];

pthread_t thread_id;

void print_greeting() {
    printf("\n************************************\n");
    printf("*          Welcome to chat         *\n");
    printf("************************************\n\n");
}

void ask_for_termination() {
    kill(getpid(), SIGINT);
}

void get_name() {
    printf("Please enter your name: ");
    fgets(name_buff, NAME_BUFF_SIZE, stdin);

    int n = 0;
    while (n < NAME_BUFF_SIZE) {
        if (name_buff[n] == '\n') {
            name_buff[n] = '\0';
            break;
        }

        n++;
    }
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
    printf("\nGood bye :)\n");
    
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

void get_named_input() {
    bzero(buff, BUFF_SIZE); 
    printf(">");

    // copy name into buffer
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

    get_name();

    printf("Hello, %s!\n", name_buff);

    // Creating socket file descriptor
    socket_fd = socket(AF_INET, SOCK_STREAM, DEFAULT_FLAGS);
    if (socket_fd < 0) { 
        perror("Error while creating a socket\n");
        exit(-1); 
    }
  
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr)); 
  
    // Filling server information 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(PORT); 
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1"); 
  
    if (connect(socket_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) { 
        perror("Cannot connect to given server.\n");
        close_socket(); 
        exit(-1); 
    }

    printf("Connected to server successfully!\n");    

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