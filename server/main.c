#include "utils.h"
#include "clients.h"

#include <ctype.h>
#include <unistd.h>
#include <stdio.h>

// #include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

void* client_runner(void* arg) {
	int client_socket = *(int*) arg;
    free(arg);

    char buffer[BUFFER_SIZE] = { 0 };

    ssize_t bytes_sent;
    ssize_t bytes_received;

    connected_user user;
    user.socket_descriptor = client_socket;

    // get user name
    while (true) {
        bytes_sent = send(client_socket, PROTOCOL_MESSAGE_NAME_GET, sizeof(PROTOCOL_MESSAGE_NAME_GET), DEFAULT_FLAGS);

        if (bytes_sent <= 0) {
            perror("Zero bytes sent. Closing socket.\n");
            close(client_socket);
	        pthread_exit(0);
        }

        bytes_received = recv(client_socket, &buffer, sizeof(buffer), DEFAULT_FLAGS);
        if (bytes_received <= 0) {
            perror("Zero bytes received. Closing socket.\n");
            close(client_socket);
            pthread_exit(0);
        }

        strip_string(buffer);
        if (strlen(buffer) <= 0) {
            continue;
        }

        if (is_name_free(buffer)) {
            strcpy(user.name, buffer);
            Status status = save_client(&user);
            if (status == SUCCESS) {
                bytes_sent = send(client_socket, PROTOCOL_MESSAGE_NAME_OK, sizeof(PROTOCOL_MESSAGE_NAME_OK) - 1, DEFAULT_FLAGS);
                if (bytes_sent <= 0) {
                    perror("Zero bytes sent. Closing socket.\n");
                    remove_client(&user);
                    pthread_exit(0);
                }
                break;
            } else {
                remove_client(&user);
	            pthread_exit(0);
            }
        }
    }

    printf("User %s ready for communication\n", user.name);

    while (true) {
        bytes_received = recv(client_socket, &buffer, sizeof(buffer), DEFAULT_FLAGS);
        strip_string(buffer);
        if (bytes_received <= 0) {
            printf("Client left. Exiting thread...\n");
            break;
        }
        char message[BUFFER_SIZE];
        snprintf(message, BUFFER_SIZE, "PRANESIMAS%s: %s\n", user.name, buffer);
        send_to_all_clients(message);
    }

    remove_client(&user);
	pthread_exit(0);
}

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
    int server_socket;
    struct addrinfo *info_node;
    for(info_node = server_info; info_node != NULL; info_node = info_node->ai_next) {
        /**
         * Create a socket for communication from given arguments and return its descriptor
         */ 
        server_socket = socket(info_node->ai_family, info_node->ai_socktype, info_node->ai_protocol);
        if (server_socket == -1) {
            perror("Error while creating server socket\n");
            continue;
        }

        /**
         * Set options on socket.
         * 
         * SOL_SOCKET - level where should options be applied. In this case socket level.
         * SO_REUSEADDR - allow to reuse local addresses
         */ 
        int yes = 1;
        if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            close(server_socket);
            perror("Error while setting socket options\n");
            exit(1);
        }

        /**
         * Assign a local protocol address to socket
         */
        if (bind(server_socket, info_node->ai_addr, info_node->ai_addrlen) == -1) {
            close(server_socket);
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
     * and set the waiting queue limit for waiting connections.
     */
    if (listen(server_socket, MAX_CLIENTS) == -1) {
        perror("Error while listening\n");
        close(server_socket);
        exit(1);
    }

    printf("Server is listening on port %s\n", port_buffer);

    while (true) {
        /**
         * 
         */
        int client_socket = accept(server_socket, NULL, NULL);
        if (client_socket <= 0) {
            perror("Error while accepting connection\n");
            break;
        }

        printf("New client connected\n");

        int *socket_ptr = malloc(sizeof(int));
        if (socket_ptr == NULL) {
            perror("Error while allocating memory\n");
            close(client_socket);
            break;
        }
        *socket_ptr = client_socket;

        if (is_room_full()) {
            printf("Chat room is full. Closing socket.\n");
            // possible to send Error message to client
            close(client_socket);
            continue;
        }

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, client_runner, socket_ptr) != 0) {
            perror("Error while creating thread\n");
            close(client_socket);
            break;
        }

        if (pthread_detach(thread_id) != 0) {
            perror("Error while detaching thread\n");
            close(client_socket);
            break;
        }
    }

    // close socket
    close(server_socket);

    return 0;
}
