#include "utils.h"

#include <strings.h>
#include <stdio.h> 

void strip_string(char* str) {
    str[strcspn(str, "\n")] = '\0';
}

void get_server_port(char* message, char* buff, size_t buff_size) {
    puts(message);
    fgets(buff, buff_size, stdin);
    strip_string(buff);
}

void uppercase(char* string, int length) {
    for (int i = 0; i < length; ++i) {
        string[i] = toupper(string[i]);
    }
}

int next_index(int* array, int size) {
    for (int i = 0; i < size; ++i) {
        if (array[i] == NULL) {
            return i;
        }
    }

    return -1;
}