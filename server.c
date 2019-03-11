#include "utils.h"

void* client_runner(void* arg) {
	int client_socket = *(int*) arg;
    free(arg);

    char buffer[BUFFER_SIZE] = { 0 };

    ssize_t bytes_sent;
    ssize_t bytes_received;

    connected_user user;
    user.socket_descriptor = client_socket;

    // get user name
    while (TRUE) {
        bytes_sent = send(client_socket, PROTOCOL_MESSAGE_GET_NAME, sizeof(PROTOCOL_MESSAGE_GET_NAME), DEFAULT_FLAGS);

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
            STATUS status = save_client(&user);
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

    while (TRUE) {
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
    
    // get port from the user
    char port_buffer[7];
    get_server_port("Enter server port to listen: ", port_buffer);

    struct addrinfo hints;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo *server_info;
    int addrinfo_status = getaddrinfo(NULL, port_buffer, &hints, &server_info);
    if (addrinfo_status != 0) {
        fprintf(stderr, "Error while getaddrinfo: %s\n", gai_strerror(addrinfo_status));
        exit(1);
    }

    // create a TCP socket
    int server_socket;
    struct addrinfo *info_node;
    for(info_node = server_info; info_node != NULL; info_node = info_node->ai_next) {
        if ((server_socket = socket(info_node->ai_family, info_node->ai_socktype,
                info_node->ai_protocol)) == -1) {
            perror("Error while creating server socket\n");
            continue;
        }

        int yes = 1;
        if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("Error while setting socket options\n");
            exit(1);
        }

        if (bind(server_socket, info_node->ai_addr, info_node->ai_addrlen) == -1) {
            close(server_socket);
            perror("Error while binding socket\n");
            continue;
        }

        break;
    }

    if (info_node == NULL)  {
        perror("Error while trying to connect \n");
        exit(1);
    }

    freeaddrinfo(server_info); // all done with this structure

    // make socket passive and listen for connection
    if (listen(server_socket, MAX_CLIENTS) == -1) {
        perror("Error while listening\n");
        close(server_socket);
        exit(1);
    }

    printf("Server is listening on port %s\n", port_buffer);

    while (TRUE) {
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
