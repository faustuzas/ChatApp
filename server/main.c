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

void close_socket(int socket) {
    /**
     * Disable writing and reading from the socket.
     */
    shutdown(socket, SHUT_RDWR);

    /**
     * Delete the socket's file descriptor.
     */
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

/**
 * Extracts first connection request from the queue of pending connections
 * and create socket for it.
 */ 
int accept_connection(int server_fd) {
    int client_fd = accept(server_fd, NULL, NULL);
    if (client_fd <= 0) {
        perror("Error while accepting a connection\n");
        return -1;
    }

    int next_i = next_index(client_fds, MAX_CLIENTS);
    if (next_i == -1) {
        printf("Only %d clients can connect simultaneously.\n", MAX_CLIENTS);
        close_socket(client_fd);
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

/**
 * Add client sockets' file descriptors into read descriptors set
 * which will be checked by select() function.
 */
void register_client_read_handlers(fd_set* set) {
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (client_fds[i] == NULL) {
            continue;
        }

        FD_SET(*client_fds[i], set);
        printf("Registering client \"%d\" socket\n", *client_fds[i]);
    }
}

void publish_message(int sender, char* message) {
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (client_fds[i] == NULL || *client_fds[i] == sender) {
            continue;
        }

        /**
         * Send provided message to the client.
         * 
         * If an error ocurred close the socket.
         */
        if (send(*client_fds[i], message, strlen(message) + 1, DEFAULT_FLAGS) == -1) {
            perror("Error while sending\n");
            close_client_socket(*client_fds[i]);
        }
    }
}

/**
 * Signal handler function
 */
void signal_handler(int sig) {
    print_goodbye();

    printf("Connected clients: \n");

    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (client_fds[i] == NULL) {
            continue;
        }

        printf("  * %d\n", *client_fds[i]);
    }
    
    close_socket(server_fd);
    close_all_client_sockets();

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

void e_exit(char* message) {
    perror(message);
    exit(-1);
}

int main() {
    init_signal_handlers();

    print_greeting();

    /**
     * Create a socket which will be used to listen for incoming connections.
     * 
     * AF_INET - an address family that is used to designate the type of addresses that the socket can communicate with.
     * In this case its IPv4.
     * 
     * SOCK_STREAM - a connection-based protocal type. 	
     * The connection has to be established between two parties to communicate.	
     * Is used alongside TCP protocol.	
     * 	
     * SOCK_DGRAM - a datagram-based protocol type.	
     * No connection needed. You just "throw" the data into the receiver and hope it gets it.	
     * Is used alongside UDP protocol.	
     */
    server_fd = socket(AF_INET, SOCK_STREAM, DEFAULT_FLAGS);
    if (server_fd <= 0) {
        e_exit("Socket creation failed");
    }

    int port = get_server_port();

    /**
     * Create and initialize a structure for holding internet address.
     */
    struct sockaddr_in servaddr; 
    bzero(&servaddr, sizeof(servaddr));

    /**
     * Address family that the socket is going to bind for. 
     * In this case IPv4.
     */
    servaddr.sin_family = AF_INET; 

    /**
     * Instruction to bind to ALL local network interfaces.
     */
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 

    /**
     * Convert the port bytes from host to network byte order and assign it to address. 
     */
    servaddr.sin_port = htons(port);

    /**
     * Assign a local protocol address to the socket.
     */ 
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

    while (true) {
        printf("************ LOOP ************\n");

        FD_ZERO(&read_fds);

        /**
         * Add interesting file desctriptors into read descriptors set 
         * which will be checked for activity by select() function.
         */
        printf("Registering server socket.\n");
        FD_SET(server_fd, &read_fds);
        register_client_read_handlers(&read_fds);

        /**
         * Check if there is any activity in given read descriptors set.
         * 
         * After function call descriptor set will contain only the descriptors
         * which has some activity.
         */
        printf("Waiting for activity...\n");
        int a = select(FD_SETSIZE, &read_fds, NULL, NULL, NULL);
        if (a == 0) {
            continue;
        }

        printf("Select found activity in %d sockets.\n", a);

        /**
         * Check if there was any activity in server socket.
         */
        if (FD_ISSET(server_fd, &read_fds)) {
            printf("Found activity in server socket\n");
            int new_client = accept_connection(server_fd);
            if (new_client > 0) {
                printf("New client connected with fd \"%d\"!\n", new_client);
            }
        }

        /**
         * Iterate throught client sockets' descriptors and check for activity.
         */
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (client_fds[i] == NULL) {
                continue;
            }

            int client_socket = *client_fds[i];
            if (!FD_ISSET(client_socket, &read_fds)) {
                continue;
            }

            printf("Found activity at \"%d\" socket\n", client_socket);

            /**
             * Because activity was detected in this socket, read from it.
             */
            bzero(buff, BUFF_SIZE);
            int received_bytes = recv(client_socket, &buff, BUFF_SIZE, DEFAULT_FLAGS);
            if (received_bytes <= 0) {
                close_client_socket(client_socket);
                printf("Closing client socket...\n");
                continue;
            }

            printf("Received message: %s\n", buff);

            /**
             * Publish the message received to other clients.
             */
            publish_message(client_socket, buff);
        }
    }
}
