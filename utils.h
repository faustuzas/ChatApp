#ifndef UTILS_HEADER
#define UTILS_HEADER

#include <strings.h>
#include <stdlib.h> 
#include <stdio.h> 

#include <sys/types.h>
#include <sys/socket.h>

#include <unistd.h>

#include <ctype.h>
#include <netinet/in.h>
#include <netdb.h>

#include <pthread.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 256

#define DEFAULT_FLAGS 0

typedef int BOOL;
#define TRUE 1
#define FALSE 0

typedef int STATUS;
#define SUCCESS 1
#define ERROR 0

typedef struct connected_user {
    int socket_descriptor;
    char* name;
} connected_user;

static const char* PROTOCOL_MESSAGE_GET_NAME = "ATSIUSKVARDA\n";
static const char* PROTOCOL_MESSAGE_NAME_OK = "VARDASOK\n";

void strip_string(char* str);

void get_server_port(char* message, char* port_buffer);

void uppercase(char* string, int length);

STATUS save_client(connected_user *user);

BOOL is_name_free(char* name);

BOOL is_room_full();

#endif