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

#define PORT 5001

int socket_fd = -1;

char nameBuff[50]
char buff[BUFF_SIZE];

pthread_t* thread_id = NULL;

void ask_for_termination() {
    kill(getpid(), SIGINT);
}

void close_socket() {
    if (socket_fd > 0) {
        printf("\nClosing server socket...\n");
        shutdown(socket_fd, SH_RDWR);
        close(socket_fd);
    }
}

void* listening_runner(void* arg) {
    char buff[BUFF_SIZE];

    while (1) {
        bzero(buff, BUFF_SIZE);
        int received_bytes = recv(sockfd, &buff, BUFF_SIZE, 0);
        if (received_bytes <= 0) {
            ask_for_termination();
            return;
        }

        printf("%s\n", buff);
    }
}

/**
 * Signal handler function
 */
void signal_handler(int sig) {
    printf("Good bye :)\n");
    
    close_socket();

    if (thread_id != NULL) {
        pthread_join(*thread_id, NULL);
    }

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
    int name_len = strlen(nameBuff);
    buff[0] = "["
    memcpy(buff + 1, nameBuff, name_len);
    buff[name_len] = "]";

    int n = name_len + 1; 
    while ((buff[n++] = getchar()) != '\n') 
        ; 

    buff[n - 1] = '\0';    
}

int main() { 
    init_signal_handlers();

    print_greeting();

    get_name();

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

    if (pthread_create(thread_id, NULL, listening_runner, NULL) != 0) {
        perror("Error while creating a thread");
        close_socket(); 
        exit(-1);
    }

    while(true) { 
        get_named_input();

        send(sockfd, buff, strlen(buff), 0);
    } 
} 