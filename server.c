#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <zlib.h>
#include <time.h>

#define BUFFER_SIZE 1024
#define PORT 9001
#define FILE_DIRECTORY "/home/vboxuser/Desktop/COMP8567/"

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

void compressAndSendFiles(int client_socket, const char *directory, off_t size1, off_t size2) {
   /* DIR *dir;
    struct dirent *entry;
    struct stat file_stat;
    char filepath[PATH_MAX];
    char buffer[BUFFER_SIZE];
    gzFile archive;
    
    archive = gzopen("temp.tar.gz", "w");
    if (!archive) {
        perror("Failed to create archive");
        exit(EXIT_FAILURE);
    }

    if ((dir = opendir(directory)) == NULL) {
        perror("Error opening directory");
        exit(EXIT_FAILURE);
    }

    while ((entry = readdir(dir)) != NULL) {
        snprintf(filepath, sizeof(filepath), "%s/%s", directory, entry->d_name);
        
        if (stat(filepath, &file_stat) == 0 && S_ISREG(file_stat.st_mode)) {
            off_t file_size = file_stat.st_size;
            if (file_size >= size1 && file_size <= size2) {
                int file_fd = open(filepath, O_RDONLY);
                ssize_t bytes_read;

                gzprintf(archive, "%s\n", entry->d_name);

                while ((bytes_read = read(file_fd, buffer, sizeof(buffer))) > 0) {
                    gzwrite(archive, buffer, bytes_read);
                }

                close(file_fd);
            }
        }
    }

    closedir(dir);
    gzclose(archive);

    // Send the compressed archive to the client
    int archive_fd = open("temp.tar.gz", O_RDONLY);
    off_t archive_size = lseek(archive_fd, 0, SEEK_END);
    lseek(archive_fd, 0, SEEK_SET);

    sendfile(client_socket, archive_fd, NULL, archive_size);

    close(archive_fd);
    remove("temp.tar.gz");*/
}

void pclientrequest(int clientSocket) {
    char buffer[255];
    int bytesRead;

    for (;;) {
        // Receive command from the client
        bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesRead <= 0) {
            // Handle client disconnection or error
            break;
        }
	
	// Process the command (you need to implement the logic)
        // For example, you can print the received command
        printf("Received command from client: %s\n", buffer);

	if (strncmp(buffer, "getfn", 5) == 0) {
            // Extract filename from the command
            char filename[256];
            sscanf(buffer, "getfn %s", filename);

            // Send file information to the client
            send_file_info(clientSocket, filename);
        }

	else if (strncmp(buffer, "getfz", 5) == 0) {
	int size1, size2;
		const char *home_directory = getenv("HOME");
    if (home_directory == NULL) {
        perror("Unable to determine home directory");
        exit(EXIT_FAILURE);
    }
    sscanf(buffer, "%*s %d %d", &size1, &size2);
    compressAndSendFiles(clientSocket, home_directory, size1, size2);
    
	}
	
	// Check if the received command is "quitc"
        else if (strcmp(buffer, "quitc") == 0) {
            // If the command is "quitc", exit the loop and close the connection
            break;
        }
        
        // Send the result back to the client (you need to implement the logic)
        // For example, you can send a response message
        const char* responseMsg = "Command processed successfully";
        send(clientSocket, responseMsg, strlen(responseMsg), 0);

        
    }

    // Close the client socket
    close(clientSocket);
}

int main(int argc, char const* argv[]) {
    // Create server socket
        int servSockD = socket(AF_INET, SOCK_STREAM, 0);

    // Define server address
    struct sockaddr_in servAddr;

    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(9002);
    servAddr.sin_addr.s_addr = INADDR_ANY;

    // Bind socket to the specified IP and port
    bind(servSockD, (struct sockaddr*)&servAddr, sizeof(servAddr));

    // Listen for connections
    // Increased the backlog to 5 to allow multiple clients
    listen(servSockD, 5);  
    printf("Listening for clients\n");

    // Infinite loop for handling client connections
    for (;;) {
    printf("Inside for\n");
        // Accept a new connection
        int clientSocket = accept(servSockD, NULL, NULL);
printf("after clientSocket");
        // Fork a new process to handle the client request
        pid_t childPid = fork();

        if (childPid == 0) {
            // In child process
            close(servSockD);  // Close the server socket in the child process
            pclientrequest(clientSocket);
            exit(0);  // Exit the child process after handling the client request
        } else {
            // In parent process
            close(clientSocket);  // Close the client socket in the parent process
        }
    }

    return 0;
}
