#include <string.h> 
#include <ctype.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <signal.h>

#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#include <pthread.h>

#define BUFF_SIZE 1024
#define PORT 5001
#define MAXLINE 512

int sockfd;

pthread_t thread_id;

void* listening_runner(void* arg) {
    char buff[BUFF_SIZE];

    while (1) {
        bzero(buff, BUFF_SIZE);
        int received_bytes = recv(sockfd, &buff, BUFF_SIZE, 0);
        if (received_bytes <= 0) {
            printf("Closing server socket...\n");
            return NULL;
        }

        printf("\nReceived from server: %s\n", buff);
    }
}

int main() { 
    struct sockaddr_in servaddr; 
  
    // Creating socket file descriptor 
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) { 
        printf("socket creation failed"); 
        exit(0); 
    } 
  
    memset(&servaddr, 0, sizeof(servaddr)); 
  
    // Filling server information 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(PORT); 
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1"); 
  
    if (connect(sockfd, (struct sockaddr*)&servaddr,  
                             sizeof(servaddr)) < 0) { 
        printf("\n Error : Connect Failed \n"); 
        return 1;
    }

    if (pthread_create(&thread_id, NULL, listening_runner, NULL) != 0) {
        perror("Error while creating a thread");
        // close socket
        exit(-1);
    }

    char buff[MAXLINE]; 
    int n; 
    for (;;) { 
        bzero(buff, sizeof(buff)); 
        printf("Enter the string : "); 
        n = 0; 
        while ((buff[n++] = getchar()) != '\n') 
            ; 

        buff[n - 1] = '\0';    
        send(sockfd, buff, strlen(buff), 0);
        
        if ((strncmp(buff, "exit", 4)) == 0) { 
            printf("Client Exit...\n"); 
            close(sockfd);
            pthread_join(thread_id, NULL);
            break; 
        } 
    } 
} 