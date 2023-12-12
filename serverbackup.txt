#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <time.h>

#define PORT 12345
#define FILE_DIRECTORY "/home/janvip/Desktop"

void send_file_info(int clientSocket, const char* filename) {
    char path[256];
    sprintf(path, "%s/%s", FILE_DIRECTORY, filename);

    // Print the constructed file path for debugging
    printf("Constructed file path: %s\n", path);

    struct stat file_stat;
    if (stat(path, &file_stat) == 0) {
        char info[1024];
        // Remove newline character from ctime result
        char* modifiedTime = ctime(&file_stat.st_mtime);
        modifiedTime[strcspn(modifiedTime, "\n")] = '\0';

        snprintf(info, sizeof(info), "File: %s\nSize: %ld bytes\nLast modified: %s\nPermissions: %o",
                 filename, file_stat.st_size, modifiedTime, file_stat.st_mode & 0777);

        send(clientSocket, info, strlen(info) + 1, 0);
    } else {
        // Print an error message for debugging
        perror("Error getting file information");
        send(clientSocket, "File not found", sizeof("File not found"), 0);
    }
}

void pclientrequest(int clientSocket) {
    char command[1024];

    while (1) {
        // Receive command from the client
        if (recv(clientSocket, command, sizeof(command), 0) <= 0) {
            break;
        }

        // Print the received command for debugging
        printf("Received command: %s\n", command);

        // Process the command and perform required actions
        if (strncmp(command, "getfn", 5) == 0) {
            // Extract filename from the command
            char filename[256];
            sscanf(command, "getfn %s", filename);

            // Send file information to the client
            send_file_info(clientSocket, filename);
        }
        // Add other command processing logic here
    }

    // Close the client socket
    close(clientSocket);
}

int main() {
    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addrLen = sizeof(clientAddr);

    // Create socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    // Bind the socket
    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("Error binding");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(serverSocket, 5) == -1) {
        perror("Error listening");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    while (1) {
        // Accept a client connection
        clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &addrLen);
        if (clientSocket == -1) {
            perror("Error accepting connection");
            continue;
        }

        // Fork a process to handle the client request
        if (fork() == 0) {
            close(serverSocket); // Child process doesn't need the listening socket
            pclientrequest(clientSocket);
            close(clientSocket);
            exit(EXIT_SUCCESS);
        } else {
            close(clientSocket); // Parent process doesn't need the client socket
        }
    }

    close(serverSocket);
    return 0;
}

