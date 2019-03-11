#include "utils.h"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
connected_user *connected_users[MAX_CLIENTS] = { NULL };

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

STATUS save_client(connected_user *user) {
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (connected_users[i] == NULL) {
            connected_users[i] = user;
            return SUCCESS;
        }
    }
    pthread_mutex_unlock(&mutex);

    return ERROR;
}

BOOL is_room_full() {
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

BOOL is_name_free(char* name) {
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        connected_user* cu = connected_users[i];
        if (cu != NULL) {
            if (strncmp(cu->name, name, BUFFER_SIZE) == 0) {
                return FALSE;
            }
        }
    }
    pthread_mutex_unlock(&mutex);

    return TRUE;
}