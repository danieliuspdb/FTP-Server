#ifdef _WIN32
#include <winsock2.h>
#define socklen_t int
#else
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#include <ctype.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFFLEN 1024

void receiveFile(int c_socket, const char *filePath) {
    printf("ZDRV1.\n");
    FILE *file = NULL;
    file = fopen("server_files/labas.txt", "wb");
    if (!file) {
        perror("ERROR: Could not open file for writing");
        exit(1);
    }
    printf("ZDRV2.\n");
    // Receive file size from the client
    char fileSize1[BUFFLEN];
    int recvResult = recv(c_socket, &fileSize1, sizeof(fileSize1), 0);

    long fileSize = atol(fileSize1);
    printf("server side filesize: %ld", fileSize);
    printf("Receive: %d", recvResult);
    if (recvResult < 0) {
        perror("ERROR: Failed to receive file size");
        fclose(file); // Close the file before exiting
        exit(1);
    } else if (recvResult == 0) {
        printf("Connection closed by the client.\n");
        fclose(file); // Close the file before exiting
        exit(1);
    }

    printf("ZDRV3.\n");
    // Receive and save file data in chunks
    char buffer[BUFFLEN];
    size_t bytesReceived;
    while (fileSize > 0) {

        bytesReceived = recv(c_socket, buffer, sizeof(buffer), 0);
        if (bytesReceived < 0) {
            perror("ERROR: Failed to receive file data");
            fclose(file); // Close the file before exiting
            exit(1);
        } else if (bytesReceived == 0) {
            printf("Connection closed by the client.\n");
            fclose(file); // Close the file before exiting
            exit(1);
        }
        printf("bufferis: %s", buffer);
        fwrite(buffer, 1, fileSize, file);
        fileSize -= bytesReceived;
    }
    printf("ZDRV4.\n");
    fclose(file);
}


void receiveFilet(const char *filename) {
    // Construct the path to the client file
    char client_filepath[] = "client_files/";
    //snprintf(client_filepath, sizeof(client_filepath), "client_files/%s", filename);
    strcat(client_filepath, filename);
    int idxToDel = 13; 
    memmove(&client_filepath[idxToDel], &client_filepath[idxToDel + 1], strlen(client_filepath) - idxToDel);   
    client_filepath[24] = '\0'; // Null-terminate the string at the 23rd character 

    printf("pirmas: %s antras: %lu", client_filepath, sizeof(client_filepath));
    printf("************");
    printf("pirmas: %s antras: %lu", "client_files/labas.txt", sizeof("client_files/labas.txt"));
    FILE *file_client = fopen(client_filepath, "rb");
    if (!file_client) {
        perror("Error opening client file for reading");
        exit(EXIT_FAILURE);
    }

    FILE *file_server = fopen("server_files/labas.txt", "wb");
    if (!file_server) {
        perror("Error opening server file for writing");
        fclose(file_client);
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFLEN];
    size_t bytes_read;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file_client)) > 0) {
        if (fwrite(buffer, 1, bytes_read, file_server) != bytes_read) {
            perror("Error writing to server file");
            fclose(file_client);
            fclose(file_server);
            exit(EXIT_FAILURE);
        }
    }

    if (ferror(file_client)) {
        perror("Error reading from client file");
    }

    fclose(file_client);
    fclose(file_server);

    printf("File successfully transferred from client to server.\n");
}

int main(int argc, char *argv[]) {
#ifdef _WIN32
    WSADATA data;
#endif
    unsigned int port;
    int l_socket;
    int c_socket;

    struct sockaddr_in6 servaddr6;
    struct sockaddr_in clientaddr;
    socklen_t clientaddrlen;

    int s_len;
    int r_len;
    char buffer[BUFFLEN];
    
    if (argc != 2){
        printf("USAGE: %s <port>\n", argv[0]);
        exit(1);
    }

    port = atoi(argv[1]);

    if ((port < 1) || (port > 65535)){
        printf("ERROR #1: invalid port specified.\n");
        exit(1);
    }

#ifdef _WIN32
    WSAStartup(MAKEWORD(2,2),&data);    
#endif

    if ((l_socket = socket(AF_INET6, SOCK_STREAM, 0)) < 0){
        fprintf(stderr,"ERROR #2: cannot create listening socket.\n");
        exit(1);
    }
    
    memset(&servaddr6, 0, sizeof(servaddr6));
    servaddr6.sin6_family = AF_INET6;

    servaddr6.sin6_addr = in6addr_any;
    servaddr6.sin6_port = htons(port);
    
    if (bind(l_socket, (struct sockaddr *)&servaddr6, sizeof(servaddr6)) < 0){
        fprintf(stderr,"ERROR #3: bind listening socket.\n");
        exit(1);
    }

    if (listen(l_socket, 5) < 0){
        fprintf(stderr,"ERROR #4: error in listen().\n");
        exit(1);
    }

    for(;;){
        memset(&clientaddr, 0, sizeof(clientaddr));
        memset(&buffer, 0, sizeof(buffer));

        clientaddrlen = sizeof(struct sockaddr);
        if ((c_socket = accept(l_socket,
            (struct sockaddr*)&clientaddr, &clientaddrlen)) < 0){
            fprintf(stderr,"ERROR #5: error occurred accepting connection.\n");
            exit(1);
        }

        s_len = recv(c_socket, buffer, 6, 0);

        if (s_len > 0) {
            //buffer[strcspn(buffer, "\n")] = '\0';
            size_t len = strcspn(buffer, "\r\n");
            buffer[len] = '\0'; 
            printf("ZDRV0.\n");
            printf("%s", buffer);
            printf("ZDRV01.\n");
            if (strncmp(buffer, "yess", strlen("yess")) == 0){
                // If the received data is a file path, receive the file
                printf("ZDRV.\n");
                receiveFile(c_socket, buffer);
                printf("File received and saved.\n");
            }
            else if (strncmp(buffer, "fail", strlen("fail")) == 0){
                // If the received data is a file path, receive the file
                printf("ZDRV.\n");
                char filename[10];
                ssize_t recv_result;

                recv_result = recv(c_socket, filename, sizeof(filename), 0);
                    if (recv_result == -1) {
                        perror("Error receiving filename");
                        exit(EXIT_FAILURE);
                    } else if (recv_result == 0) {
                        printf("Connection closed by the client.\n");
                        exit(EXIT_FAILURE);
                    }
                    printf("numeris: %zd", recv_result);
                    // Null-terminate the received filename
                    filename[recv_result] = '\0';

                    // Call receiveFilet function with the received filename
                    receiveFilet(filename);       
                    printf("File received and saved.\n");
            } else {
                // If not a file path, process the message
                for (int i = 0; i < s_len; i++) {
                    buffer[i] = toupper((unsigned char)buffer[i]);
                }

                r_len = send(c_socket, buffer, s_len, 0);

                printf("IP: %s Sent: %d Received: %d\n", inet_ntoa(clientaddr.sin_addr),
                    s_len, r_len
                );
            }
        }

        close(c_socket);
    }

    return 0;
}
