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
#define BUFF_SIZE 512

int main() {
    char port_buffer[7];

    // TODO: Port validation
    get_server_port("Enter server port to listen: ", port_buffer);

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
        fprintf(stderr, "Error while getaddrinfo: %s\n", gai_strerror(addrinfo_status));
        exit(1);
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
            exit(1);
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
        perror("Error while creating and configuring the socket\n");
        exit(1);
    }

    /**
     * Free addrinfo structure and all underlying pointers 
     */
    freeaddrinfo(server_info);

    /**
     * Specify the willingness to accept incoming connections
     * and set the waiting queue limit.
     */
    if (listen(server_socket_fd, MAX_CLIENTS) == -1) {
        perror("Error while listening\n");
        close(server_socket_fd);
        exit(1);
    }

    printf("Server is listening on port %s\n", port_buffer);

    fd_set read_fds;
    FD_ZERO(&read_fds);

    int* client_sockets[MAX_CLIENTS] = { NULL }; 

    char buff[BUFFER_SIZE];

    while (true) {
        FD_SET(server_socket_fd, &read_fds);

        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (client_sockets[i] == NULL) {
                continue;
            }

            FD_SET(*client_sockets[i], &read_fds);
        }

        // refactor to not use FD_SETSIZE
        if (select(FD_SETSIZE, &read_fds, NULL, NULL, NULL) == 0) {
            continue;
        }

        if (FD_ISSET(server_socket_fd, &read_fds)) {
            int client_socket = accept(server_socket_fd, NULL, NULL);
            if (client_socket <= 0) {
                perror("Error while accepting connection\n");
                break;
            }

            for (int i = 0; i < MAX_CLIENTS; ++i) {
                if (client_sockets[i] == NULL) {
                    client_sockets[i] = &client_socket;
                    break;
                }
            }

            printf("New client connected\n");
        }

        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (client_sockets[i] == NULL) {
                continue;
            }

            int client_socket = *client_sockets[i];
            if (!FD_ISSET(client_socket, &read_fds)) {
                continue;
            }

            if (recv(client_socket, &buff, BUFF_SIZE, DEFAULT_FLAGS) == -1) {
                close(client_socket);
                client_sockets[i] = NULL;
                continue;
            }

            printf("Received message: %s\n", buff);

            for (int i = 0; i < MAX_CLIENTS; ++i) {
                // if (client_sockets[i] == NULL || *client_sockets[i] == client_socket) {
                if (client_sockets[i] == NULL) {
                    continue;
                }

                if (send(*client_sockets[i], buff, BUFF_SIZE, DEFAULT_FLAGS) == -1) {
                    perror("Error while sending\n");
                    close(*client_sockets[i]);
                    client_sockets[i] = NULL;
                    continue;
                } else {
                    printf("Message sent\n");
                }
            }
        }
    }

    // close socket
    close(server_socket_fd);

    return 0;
}
