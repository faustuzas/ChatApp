#ifndef UTILS_HEADER
#define UTILS_HEADER

#define DEFAULT_FLAGS 0

typedef int bool;
#define true 1
#define false 0

typedef int Status;
#define SUCCESS 1
#define ERROR 0

int get_server_port();

void print_greeting();

void print_goodbye();

int next_index(int* array[], int size);

#endif