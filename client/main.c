#include <string.h> 
#include <ctype.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#define PORT 5000 
#define MAXLINE 512 

int main() 
{ 
    int sockfd; 
    char buffer[MAXLINE]; 
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

    memset(buffer, 0, sizeof(buffer)); 
    strcpy(buffer, "Hello Server"); 
    printf("Sending message...\n");
    write(sockfd, buffer, sizeof(buffer)); 
    printf("Sent message\n");
    printf("Message from server: "); 
    read(sockfd, buffer, sizeof(buffer)); 
    puts(buffer); 
    close(sockfd); 
} 