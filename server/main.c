#include "utils.h"

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

#define MAX_CLIENTS 10
#define CONNECTION_QUEUE_SIZE 5
#define BUFF_SIZE 1024

char buff[BUFF_SIZE];

int server_fd;
int* client_fds[MAX_CLIENTS] = { NULL };

int accept_connection(int server_fd) {
    int client_fd = accept(server_fd, NULL, NULL);
    if (client_fd <= 0) {
        perror("Error while accepting a connection\n");
        return -1;
    }

    int next_i = next_index(client_fds, MAX_CLIENTS);
    if (next_i == -1) {
        printf("Only %d clients can connect simultaneously.\n", MAX_CLIENTS);
        return -1;
    }

    int* fd_ptr = malloc(sizeof(int));
    if (fd_ptr == NULL) {
        perror("Failed allocating memory.\n");
        return -1;
    }

    *fd_ptr = client_fd;
    client_fds[next_i] = fd_ptr;

    return client_fd;
}

void register_client_read_handlers(fd_set* set) {
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (client_fds[i] == NULL) {
            continue;
        }

        FD_SET(*client_fds[i], set);
    }
}

void close_socket(int socket) {
    shutdown(socket, SHUT_WR);
    close(socket);
}

void close_client_socket(int socket) {
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (client_fds[i] == NULL || *client_fds[i] != socket) {
            continue;
        }

        close_socket(socket);

        free(client_fds[i]);
        client_fds[i] = NULL;
    }
}

void close_all_client_sockets() {
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (client_fds[i] == NULL) {
            continue;
        }
        
        close_socket(*client_fds[i]);

        free(client_fds[i]);
        client_fds[i] = NULL;
    }
}

void publish_message(int sender, char* message) {
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (client_fds[i] == NULL /*|| *client_fds[i] == sender*/) {
            continue;
        }

        if (send(*client_fds[i], message, strlen(message), DEFAULT_FLAGS) == -1) {
            perror("Error while sending\n");
            close_client_socket(*client_fds[i]);
        }
    }
}

void signal_handler(int sig) {
    close_socket(server_fd);
    close_all_client_sockets();

    exit(0);
}

void init_signal_handlers() {
    struct sigaction sa;

    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    
    sigaction(SIGINT, &sa, NULL);
}

void e_exit(char* message) {
    perror(message);
    exit(-1);
}

int main() {
    init_signal_handlers();

    print_greeting();

    server_fd = socket(AF_INET, SOCK_STREAM, DEFAULT_FLAGS);
    if (server_fd <= 0) {
        e_exit("Socket creation failed");
    }

    int port = get_server_port();

    struct sockaddr_in servaddr; 
    bzero(&servaddr, sizeof(servaddr));

    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(port); 

    if (bind(server_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1) {
        e_exit("Socket binding failed");
    }

    /**
     * Specify the willingness to accept incoming connections
     * and set the waiting queue limit.
     */
    if (listen(server_fd, CONNECTION_QUEUE_SIZE) == -1) {
        close(server_fd);
        e_exit("Error while listening\n");
    }

    printf("Server is listening on port %d\n", port);

    fd_set read_fds;
    FD_ZERO(&read_fds);

    while (true) {
        FD_SET(server_fd, &read_fds);
        register_client_read_handlers(&read_fds);

        if (select(FD_SETSIZE, &read_fds, NULL, NULL, NULL) == 0) {
            continue;
        }

        if (FD_ISSET(server_fd, &read_fds)) {
            int new_client = accept_connection(server_fd);
            if (new_client > 0) {
                printf("New client connected!\n");
            }
        }

        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (client_fds[i] == NULL) {
                continue;
            }

            int client_socket = *client_fds[i];
            if (!FD_ISSET(client_socket, &read_fds)) {
                continue;
            }

            bzero(buff, BUFF_SIZE);
            int received_bytes = recv(client_socket, &buff, BUFF_SIZE, DEFAULT_FLAGS);
            if (received_bytes <= 0) {
                close_client_socket(client_socket);
                printf("Closing client socket...\n");
                continue;
            }

            printf("Received message: %s\n", buff);
            publish_message(client_socket, buff);
        }
    }
}
