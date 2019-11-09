#ifndef CLIENTS_HEADER
#define CLIENTS_HEADER

#include "utils.h"

#define MAX_CLIENTS 10

typedef struct connected_user {
    int socket_descriptor;
    char name[BUFFER_SIZE];
} connected_user;

Status save_client(connected_user *user);

void remove_client(connected_user *user);

bool is_name_free(char* name);

bool is_room_full();

Status send_to_all_clients(char* message);

#endif