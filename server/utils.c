#include "utils.h"

#include <strings.h>
#include <stdio.h> 

void print_greeting() {
    printf("\n************************************\n");
    printf("*      Welcome to chat server      *\n");
    printf("************************************\n\n");
}

void print_goodbye() {
    printf("\n************************************\n");
    printf("*   Shutting down the server....   *\n");
    printf("************************************\n\n");
}

int get_server_port() {
    int port;

    while(true) {
        printf("Enter port server to listen to: ");
        if (scanf("%d", &port) == 1) {
            break;
        }

        // clear stdout
        while(getchar() != '\n') ;

        printf("Please enter only port number.\n");
    }

    while(getchar() != '\n') ;  

    return port;
}

int next_index(int* array[], int size) {
    for (int i = 0; i < size; ++i) {
        if (array[i] == NULL) {
            return i;
        }
    }

    return -1;
}