#include "utils.h"

#include <string.h> 
#include <ctype.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

#define MAX_CLIENTS 10
#define CONNECTION_QUEUE_SIZE 5
#define BUFF_SIZE 512

char* buff;
int* client_fds[MAX_CLIENTS] = { NULL };

Status accept_connection(int server_fd) {
    int client_fd = accept(server_fd, NULL, NULL);
    if (client_fd <= 0) {
        perror("Error while accepting a connection\n");
        return ERROR;
    }

    int next_i = next_index(client_fds, MAX_CLIENTS);
    if (next_i == -1) {
        printf("Only %d clients can connect simultaneously.\n");
        return;
    }

    int* fd_ptr = malloc(sizeof(int*));
    if (fd_ptr == NULL) {
        perror("Failed allocating memory.\n");
        return ERROR;
    }

    *fd_ptr = client_fd;
    client_fds[next_i] = fd_ptr;

    return SUCCESS;
}

void register_client_read_handlers(fd_set* set) {
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (client_fds[i] == NULL) {
            continue;
        }

        FD_SET(*client_fds[i], set);
    }
}

void close_client_socket(int socket) {
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (client_fds[i] == NULL || *client_fds[i] != socket) {
            continue;
        }

        shutdown(socket);
        close(socket);

        free(client_fds[i]);
        client_fds[i] = NULL;
    }
}

void publish_message(int sender) {
    for (int i = 0; i < MAX_CLIENTS; ++i) {
    // if (client_sockets[i] == NULL || *client_sockets[i] == sender) {
        if (client_fds[i] == NULL) {
            continue;
        }

        if (send(*client_sockets[i], buff, BUFF_SIZE, DEFAULT_FLAGS) == -1) {
            perror("Error while sending\n");
            close_client_socket(*client_sockets[i]);
        } else {
            printf("Message sent\n");
        }
    }
}

void e_exit(char* message) {
    perror(message);
    exit(-1);
}

int main() {
    if ((buff = malloc(sizeof(char) * BUFF_SIZE)) == NULL) {
        e_exit("Unable to allocate memory for the buffer.\n");
    }

    get_server_port("Enter server port to listen: ", buff, BUFF_SIZE);

    /**
     * Struct which hints the details of address you are trying to find
     */
    struct addrinfo hints;
    memset(&hints, 0, sizeof hints);

    /**
     * Protocol family that should be used.
     * 
     * AF_UNSPEC - any family supported by the OS.
     */
    hints.ai_family = AF_UNSPEC;

    /**
     * Type of socket that is wanted.
     * 
     * SOCK_STREAM - connection-based protocal type. 
     * The connection has to be established between two parties to communicate.
     * Is used alongside TCP protocol.
     * 
     * SOCK_DGRAM - datagram-based protocol type.
     * No connection needed. You just "throw" the data into the receiver and hope it gets it.
     * Is used alongside UDP protocol.
     */
    hints.ai_socktype = SOCK_STREAM;

    /**
     * Structure that returned address should corespond to.
     * 
     * AI_PASSIVE - indicates that returned socket address structure is intended for use in a call to bind(). 
     */
    hints.ai_flags = AI_PASSIVE;

    /**
     * Struct which will hold the returned socket address
     */ 
    struct addrinfo *server_info;

    /**
     * Gets a list of IP addresses and port numbers from provided hostname and port.
     * 
     * First parameter (hostname) is NULL because this will be a server socket.
     */
    int addrinfo_status = getaddrinfo(NULL, port_buffer, &hints, &server_info);
    if (addrinfo_status != 0) {
        e_exit("Error while getaddrinfo\n");
    }

    /**
     * Iterate throught returned list and create and configure a server socket
     */ 
    int server_socket_fd;
    struct addrinfo *info_node;
    for(info_node = server_info; info_node != NULL; info_node = info_node->ai_next) {
        /**
         * Create a socket for communication from given arguments and return its descriptor
         */ 
        server_socket_fd = socket(info_node->ai_family, info_node->ai_socktype, info_node->ai_protocol);
        if (server_socket_fd == -1) {
            perror("Error while creating server socket\n");
            continue;
        }

        /**
         * Set options on a socket.
         * 
         * SOL_SOCKET - level where options should be applied. In this case socket level.
         * SO_REUSEADDR - allow to reuse local addresses
         */ 
        int yes = 1;
        if (setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            close(server_socket_fd);
            perror("Error while setting socket options\n");
            continue;
        }

        /**
         * Assign a local protocol address to the socket
         */
        if (bind(server_socket_fd, info_node->ai_addr, info_node->ai_addrlen) == -1) {
            close(server_socket_fd);
            perror("Error while binding socket\n");
            continue;
        }

        break;
    }

    if (info_node == NULL)  {
        e_exit("Error while creating and configuring the socket\n");
    }

    /**
     * Free addrinfo structure and all underlying pointers 
     */
    freeaddrinfo(server_info);

    /**
     * Specify the willingness to accept incoming connections
     * and set the waiting queue limit.
     */
    if (listen(server_socket_fd, CONNECTION_QUEUE_SIZE) == -1) {
        close(server_socket_fd);
        e_exit("Error while listening\n");
    }

    printf("Server is listening on port %s\n", port_buffer);

    fd_set read_fds;
    FD_ZERO(&read_fds);

    while (true) {
        FD_SET(server_socket_fd, &read_fds);
        register_client_read_handlers(&read_fds);

        if (select(FD_SETSIZE, &read_fds, NULL, NULL, NULL) == 0) {
            continue;
        }

        if (FD_ISSET(server_socket_fd, &read_fds)) {
            if (accept_connection(server_socket_fd) == SUCCESS) {
                printf("New client connected.\n");
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

            if (recv(client_socket, &buff, BUFF_SIZE, DEFAULT_FLAGS) == -1) {
                close_client_socket(client_socket);
                continue;
            }

            printf("Received message: %s\n", buff);
            publish_message(client_socket, &buff, BUFF_SIZE);
        }
    }

    // close socket
    close(server_socket_fd);

    return 0;
}
