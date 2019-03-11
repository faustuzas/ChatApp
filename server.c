#include "utils.h"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void* client_runner(void* arg)
{
    char buffer[BUFFER_SIZE] = { 0 };
	int client_socket = *(int*) arg;
    connected_user user;
    user.socket_descriptor = client_socket;

    // get user name
    while (TRUE) {
        ssize_t bytes_sent = send(client_socket, PROTOCAL_MESSAGE_GET_NAME, sizeof(PROTOCAL_MESSAGE_GET_NAME), DEFAULT_FLAGS);

        if (bytes_sent < 0) {
            close(client_socket);
	        pthread_exit(0);
        }

        ssize_t bytes_recevied = recv(client_socket, &buffer, sizeof(buffer), DEFAULT_FLAGS);
        if (bytes_recevied <= 0) {
            close(client_socket);
            pthread_exit(0);
        }

        BOOL name_is_taken = FALSE;
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            connected_user cu = connected_users[i];
            if (connected_user != NULL) {
                if (strcmp(cu.name, buffer) == 0) {
                    name_is_taken = TRUE;
                    break;
                }
            }
        }

        if (name_is_taken) {
            continue;
        } else {
            user.name = buffer;
            STATUS status = save_client(&user);
            if (STATUS == SUCCESS) {
                printf("User connected with name: %s", user.name);
                break;
            } else {
                close(client_socket);
	            pthread_exit(0);
            }
        }
    }

    printf("Ready for communication");

    // recv(client_socket, &client_message, sizeof(client_message), 0);

    //     if (strlen(client_message) > 0) {
    //         printf("Message from client: %s", client_message);
    //         uppercase(client_message, sizeof(client_message));

    //         char response[512];
    //         snprintf(response, sizeof response, "\nCounter: %d\n", counter++);
            
    //         strcat(response, client_message);
    //         send(client_socket, response, sizeof(client_message), 0);
    //     }

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
            perror("server: socket");
            continue;
        }

        int yes = 1;
        if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(server_socket, info_node->ai_addr, info_node->ai_addrlen) == -1) {
            close(server_socket);
            perror("server: bind");
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

    pthreads_t thread_ids[MAX_CLIENTS];
    while (TRUE) {
        int client_socket = accept(server_socket, NULL, NULL);
        if (client_socket =< 0) {
            perror("Error while accepting connection");
            break;
        }

		pthread_create(&thread_ids[i], NULL, sum_runner, &client_socket);
    }

    for (int i = 0; i < MAX_CLIENTS; ++i) {
        pthread_join(thread_ids[i], NULL);
    }

    // close socket
    close(server_socket);

    return 0;
}
