#ifndef UTILS_HEADER
#define UTILS_HEADER

#define BUFFER_SIZE 256

#define DEFAULT_FLAGS 0

typedef int bool;
#define true 1
#define false 0

typedef int Status;
#define SUCCESS 1
#define ERROR 0

#define PROTOCOL_MESSAGE_NAME_GET "NAME_GET\n"
#define PROTOCOL_MESSAGE_NAME_OK "NAME_OK\n"

void strip_string(char* str);

void get_server_port(char* message, char* port_buffer);

void uppercase(char* string, int length);

#endif