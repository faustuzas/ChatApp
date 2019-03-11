#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <unistd.h>
#include <strings.h>
#include <string.h>

#include <ctype.h>
#include <netinet/in.h>
#include <netdb.h>

#define MAX_CLIENTS 10;
#define TRUE 1

typedef struct connected_user {
    int socket_descriptor;
    char* name;
} connected_user;

void strip_string(char* str) {
    str[strcspn(str, "\n")] = 0;
}

void get_server_port(char* message, char* port_buffer) {
    puts(message);
    fgets(port_buffer, sizeof port_buffer, stdin);
    strip_string(port_buffer);
}

void uppercase(char* string, int length) {
    for (int i = 0; i < length; ++i) {
        string[i] = toupper(string[i]);
    }
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
    if (listen(server_socket, 5) == -1) {
        perror("Error while listening\n");
        close(server_socket);
        exit(1);
    }

    while (TRUE) {
        connected_user new_user;
        int client_socket = accept(server_socket, NULL, NULL);
    }

    // accept new incomming connection
    int client_socket = accept(server_socket, NULL, NULL);

    // wait for message, transform it and sent it back
    int counter = 1;
    char client_message[256] = { 0 };
    while(1) {
        recv(client_socket, &client_message, sizeof(client_message), 0);

        if (strlen(client_message) > 0) {
            printf("Message from client: %s", client_message);
            uppercase(client_message, sizeof(client_message));

            char response[512];
            snprintf(response, sizeof response, "\nCounter: %d\n", counter++);
            
            strcat(response, client_message);
            send(client_socket, response, sizeof(client_message), 0);
        }
    }

    // close socket
    close(server_socket);

    return 0;
}
