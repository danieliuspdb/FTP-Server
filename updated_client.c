#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFFLEN 1024

void sendFile(int s_socket, const char *filePath) {
    FILE *file = fopen(filePath, "rb");
    if (!file) {
        fprintf(stderr, "ERROR: Could not open file for reading.\n");
        exit(1);
    }

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    char fileSize1[BUFFLEN];
    snprintf(fileSize1, sizeof(fileSize1), "%ld", fileSize);
    fseek(file, 0, SEEK_SET);

    // Send file size to the server
    char patvirtinimas[] = "yess";
    send(s_socket, &patvirtinimas, sizeof(patvirtinimas), 0);
    send(s_socket, &fileSize1, sizeof(fileSize1), 0);
    printf("client side filesize: %s", fileSize1);
    // Send file data in chunks
    char buffer[BUFFLEN];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        send(s_socket, buffer, sizeof(buffer), 0);
    }
    fclose(file);
}

int main(int argc, char *argv[]) {
#ifdef _WIN32
    WSADATA data;
#endif
    unsigned int port;
    int s_socket;
    struct sockaddr_in servaddr;

    char buffer[BUFFLEN];

    if (argc != 3){
        fprintf(stderr,"USAGE: %s <ip> <port>\n",argv[0]);
        exit(1);
    }

    port = atoi(argv[2]);

    if ((port < 1) || (port > 65535)){
        printf("ERROR #1: invalid port specified.\n");
        exit(1);
    }

#ifdef _WIN32
    WSAStartup(MAKEWORD(2,2),&data);    
#endif

    if ((s_socket = socket(AF_INET, SOCK_STREAM,0))< 0){
        fprintf(stderr,"ERROR #2: cannot create socket.\n");
        exit(1);
    }

    memset(&servaddr,0,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    
#ifdef _WIN32
    servaddr.sin_addr.s_addr = inet_addr(argv[1]);
#else
    if ( inet_aton(argv[1], &servaddr.sin_addr) <= 0 ) {
        fprintf(stderr,"ERROR #3: Invalid remote IP address.\n");
        exit(1);
    }
#endif        

    
    if (connect(s_socket,(struct sockaddr*)&servaddr,sizeof(servaddr))<0){
        fprintf(stderr,"ERROR #4: error in connect().\n");
        exit(1);
    }

    printf("Enter the message or file path: ");
    fgets(buffer, BUFFLEN, stdin);

    if (buffer[strlen(buffer) - 1] == '\n') {
        buffer[strlen(buffer) - 1] = '\0'; // Remove the newline character
    }

    if (access(buffer, F_OK) == 0) {
        // If the input is a file path, send the file
        sendFile(s_socket, buffer);
        printf("File sent successfully.\n");
    } else {
        // If not a file path, send the message
        send(s_socket, buffer, strlen(buffer), 0);
        memset(&buffer,0,BUFFLEN);
        recv(s_socket, buffer, BUFFLEN, 0);
        printf("Server sent: %s\n", buffer);
    }

    close(s_socket);
    return 0;
}
