#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <libgen.h>
#include <fcntl.h>
#include <limits.h>
//#include <sys/sendfile.h>
#include <sys/stat.h>
//#include <zlib.h>
#include <time.h>


#define BUFFER_SIZE 1024
#define PORT 9001

void send_file_info(int clientSocket, const char *filename) {
    // Perform a recursive file search starting from the root directory ("/")
    char path[PATH_MAX];
    if (recursive_search("/", filename, path)) {
        // File found, send information to the client
        struct stat file_stat;
        if (stat(path, &file_stat) == 0) {
            char info[1024];
            int info_length = snprintf(info, sizeof(info),
                "File name: %s\nSize: %ld bytes\nLast modified: %s\nPermissions: %o",
                basename(path), file_stat.st_size, ctime(&file_stat.st_mtime), file_stat.st_mode & 0777);

            ssize_t bytes_sent = send(clientSocket, info, info_length, 0);
            if (bytes_sent == -1) {
                perror("Error sending file information");
            }
        }
    } else {
        // File not found, send an appropriate message to the client
        ssize_t bytes_sent = send(clientSocket, "File not found", sizeof("File not found"), 0);
        if (bytes_sent == -1) {
            perror("Error sending 'File not found' message");
        }
    }
}

int recursive_search(const char *current_path, const char *filename, char *result_path) {
    DIR *dir = opendir(current_path);
    if (!dir) {
        return 0; // Unable to open directory
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            // Recursive call for subdirectories
            char next_path[PATH_MAX];
            snprintf(next_path, sizeof(next_path), "%s/%s", current_path, entry->d_name);
            if (recursive_search(next_path, filename, result_path)) {
                closedir(dir);
                return 1; // File found in a subdirectory
            }
        } else if (entry->d_type == DT_REG && strcmp(entry->d_name, filename) == 0) {
            // File found
            snprintf(result_path, PATH_MAX, "%s/%s", current_path, entry->d_name);
            closedir(dir);
            return 1;
        }
    }

    closedir(dir);
    return 0; // File not found in this directory
}

void check_bytes_and_send(int client_socket, int size1, int size2){
    // Specify the output file
    const char *output_file = "received.tar.gz";
    const char *file_list = "/tmp/files_list.txt";

    // Construct the command to execute
    char command[512];
    snprintf(command, sizeof(command), "find ~ -type f -size +%dc -size -%dc -not -path '*/.cache/*' -not -path '*/.local/*' -not -path '*/.thunderbird/*' -not -path '*/.gnupg/*' -not -path '*/.config/*' > %s && tar -czf %s --files-from=%s && rm %s",
             size1, size2, file_list, output_file, file_list, file_list);

    // Execute the command
    int result = system(command);

    if (result == 0) {
        printf("Command executed successfully.\n");
    } else {
        fprintf(stderr, "Command failed.\n");
    }

   
}

void create_and_send_tar(int client_socket, const char *extensions[], int num_extensions) {
    // Get the home directory dynamically
    const char *home_dir = getenv("HOME");
    if (home_dir == NULL) {
        // Handle the case where the HOME environment variable is not set
        fprintf(stderr, "Error getting home directory.\n");
        exit(EXIT_FAILURE);
    }

    const char *tar_filename = "result.tar";

    // Build the tar command with find to include all specified file extensions and exclude hidden directories
    char tar_command[512];
    snprintf(tar_command, sizeof(tar_command), "find %s -type f \\( -not -path '%s/.cache/*' \\) -a \\( -not -path '%s/.local/*' \\)", home_dir, home_dir, home_dir);

    for (int i = 0; i < num_extensions; ++i) {
        if (i == 0) {
            snprintf(tar_command + strlen(tar_command), sizeof(tar_command) - strlen(tar_command), " \\( ");
        } else {
            snprintf(tar_command + strlen(tar_command), sizeof(tar_command) - strlen(tar_command), " -o ");
        }
        snprintf(tar_command + strlen(tar_command), sizeof(tar_command) - strlen(tar_command), "-name '*.%s'", extensions[i]);
        if (i == num_extensions - 1) {
            snprintf(tar_command + strlen(tar_command), sizeof(tar_command) - strlen(tar_command), " \\)");
        }
    }

    snprintf(tar_command + strlen(tar_command), sizeof(tar_command) - strlen(tar_command), " -print0 | tar --null -T - -rf %s -C %s", tar_filename, home_dir);

    // Execute the tar command
    int result = system(tar_command);

    if (result == 0) {
        printf("Tar file created successfully.\n");

        // Compress the tar file using gzip
        char gzip_command[256];
        snprintf(gzip_command, sizeof(gzip_command), "gzip %s", tar_filename);

        result = system(gzip_command);

        if (result == 0) {
            printf("Tar file compressed successfully.\n");
        } else {
            fprintf(stderr, "Error compressing tar file.\n");
            exit(EXIT_FAILURE);
        }
    } else {
        fprintf(stderr, "Error creating tar file.\n");
        exit(EXIT_FAILURE);
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
    sscanf(buffer, "%*s %d %d", &size1, &size2);
    check_bytes_and_send(clientSocket, size1, size2);
    
	}
    else if (strncmp(buffer, "getft", 5) == 0) {
    int count = 0;
    int offset = 0;
    char attribute[255];
    const char* extensions[3];  // Assuming a maximum of 3 extensions

    while (sscanf(buffer + offset, "%s", attribute) == 1) {
        count++;
        offset += strlen(attribute) + 1;
    }
    
        const char* tempExtensions[3];
        const char* tempBuffer = buffer + 5;  // Skip "getft" in the command

        // After the loop that populates tempExtensions array
for (int i = 0; i < count - 1 && sscanf(tempBuffer, "%s", attribute) == 1; ++i) {
    tempExtensions[i] = strdup(attribute);
    printf("tempExtensions[%d]: %s\n", i, tempExtensions[i]);
    tempBuffer += strlen(attribute) + 1;
}


        

        create_and_send_tar(clientSocket, tempExtensions, count - 1);
    
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
