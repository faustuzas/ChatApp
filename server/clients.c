#include "clients.h"

#include <pthread.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
connected_user* connected_users[MAX_CLIENTS] = { NULL };

Status save_client(connected_user *user) {
    Status status = ERROR;

    pthread_mutex_lock(&mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (connected_users[i] == NULL) {
            connected_users[i] = user;
            status = SUCCESS;
            break;
        }
    }
    pthread_mutex_unlock(&mutex);
    
    return status;
}

void remove_client(connected_user *user) {
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (connected_users[i] == user) {
            connected_users[i] = NULL;
            close(user->socket_descriptor);
            break;
        }
    }
    pthread_mutex_unlock(&mutex);
}

bool is_room_full() {
    int places_occupied = 0;

    pthread_mutex_lock(&mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (connected_users[i] != NULL) {
            places_occupied += 1;
        }
    }
    pthread_mutex_unlock(&mutex);

    return places_occupied == MAX_CLIENTS;
}

bool is_name_free(char* name) {
    bool is_free = true;
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        connected_user* cu = connected_users[i];
        if (cu != NULL) {
            if (strncmp(cu->name, name, BUFFER_SIZE) == 0) {
                is_free = false;
                break;
            }
        }
    }
    pthread_mutex_unlock(&mutex);

    return is_free;
}

Status send_to_all_clients(char* message) {
    ssize_t bytes_sent;

    pthread_mutex_lock(&mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        connected_user* user = connected_users[i];
        if (user != NULL) {
            bytes_sent = send(user->socket_descriptor, message, strlen(message), DEFAULT_FLAGS);
            if (bytes_sent <= 0) {
                perror("Zero bytes sent. Closing socket.\n");
                close(user->socket_descriptor);
                pthread_exit(0);
            }
        }
    }
    pthread_mutex_unlock(&mutex);

    return SUCCESS;
}