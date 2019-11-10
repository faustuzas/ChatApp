#include "utils.h"

#include <strings.h>
#include <stdio.h> 

void print_greeting() {
    printf("\n************************************\n");
    printf("*          Welcome to chat         *\n");
    printf("************************************\n\n");
}

void print_goodbye() {
    printf("\n************************************\n");
    printf("*             Good bye!             *\n");
    printf("************************************\n\n");
}

int get_server_port() {
    int port;

    while(true) {
        printf("Enter server port: ");
        if (scanf("%d", &port) == 1) {
            break;
        }

        // clear stdout
        while(getchar() != '\n') ;

        printf("Please enter only port number.\n");
    }

    return port;
}

void get_name(char* buff, int buff_size) {
    printf("Please enter your name: ");
    fgets(buff, buff_size, stdin);

    int n = 0;
    while (n < buff_size) {
        if (buff[n] == '\n') {
            buff[n] = '\0';
            break;
        }

        n++;
    }
}