#include "utils.h"

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

connected_user *connected_users[MAX_CLIENTS];

STATUS save_client(connected_user *user) {
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (connected_users[i] == NULL) {
            connected_users[i] = user;
            return SUCCESS;
        }
    }

    return ERROR;
}