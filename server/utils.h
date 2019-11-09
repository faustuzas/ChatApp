#ifndef UTILS_HEADER
#define UTILS_HEADER

#define DEFAULT_FLAGS 0

typedef int bool;
#define true 1
#define false 0

typedef int Status;
#define SUCCESS 1
#define ERROR 0

void strip_string(char* str);

void get_server_port(char* message, char* port_buffer);

void uppercase(char* string, int length);

int next_index(int* array, int size);

#endif