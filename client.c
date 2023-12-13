#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
//#include <sys/types.h>
#include <unistd.h>

#define BUFFER_SIZE 1024
 

void saveIntolocal(int server_socket) {
    FILE *received_file = fopen("temp.tar.gz", "wb");
    if (received_file == NULL) {
        perror("Error opening archive file");
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;

    // Receive and save the compressed archive
    while ((bytes_received = recv(server_socket, buffer, sizeof(buffer), 0)) > 0) {
        fwrite(buffer, 1, bytes_received, received_file);
    }

    fclose(received_file);
}

int main(int argc, char const* argv[]) {
    int sockD = socket(AF_INET, SOCK_STREAM, 0);
    
if (sockD == -1) {
    perror("Error creating socket");
    exit(EXIT_FAILURE);
}

    struct sockaddr_in servAddr;

    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(9002);
    servAddr.sin_addr.s_addr = INADDR_ANY;

    int connectStatus = connect(sockD, (struct sockaddr*)&servAddr, sizeof(servAddr));
    

    if (connectStatus == -1) {
        //printf("Error while connecting to the server\n");
        perror("Error while connecting to the server");
    } else {
        char strData[BUFFER_SIZE];

        while(1){
            char userInput[BUFFER_SIZE];
            printf("Enter a command:: ");
            fgets(userInput, BUFFER_SIZE, stdin);

            //Processing user inputted command
            if (strncmp(userInput, "getfn", 5) == 0) {
            
           	printf("-----User entered getfn-----\n");
                char filename[BUFFER_SIZE];
                int count = 0;
   		 int offset = 0;
   		 char attribute[255];

    		while (sscanf(userInput + offset, "%s", attribute) == 1) {
       		 count++;
       		 offset += strlen(attribute) + 1;
   		}
   		
   		printf("Number of attributes:: %d\n", count);
   		sscanf(userInput, "%*s %s", filename);
   		
   		if(count>2 || strchr(filename, '.') == NULL){
   		printf("Syntax is incorrect, usage:: getfn filename");
   		}
   		
   		else{
   		printf("Filename:: %s\n", filename);
   		send(sockD, userInput, strlen(userInput), 0);
		
		// Receive and print the server's response
        char response[1024];
        memset(response, 0, sizeof(response));

        recv(sockD, response, sizeof(response) - 1, 0);
        response[sizeof(response) - 1] = '\0';

        printf("Server response: %s\n", response);
   		}
   		continue;
            } else if (strncmp(userInput, "getfz", 5) == 0) {
                int count = 0;
   		 int offset = 0;
   		 char attribute[255];
   		 int size1, size2;
   		 //char command[10];

    		while (sscanf(userInput + offset, "%s", attribute) == 1) {
       		 count++;
       		 offset += strlen(attribute) + 1;
   		}
   		
   		printf("Number of attributes:: %d\n", count);
   		
   		//sscanf(userInput, "%19s %d %d", command, &size1, &size2);
   		sscanf(userInput, "%*s %d %d", &size1, &size2);
   		
   		if(count<3 || count>3 || size1>size2){
   		printf("Syntax is incorrect, usage:: getfz size1 size2 and size1 < size2\n");
   		}
   		
   		else{
   		printf("UserInput:: %s\n", userInput);
   		send(sockD, userInput, strlen(userInput), 0);
   		saveIntolocal(sockD);
   		}
   		
            } else if (strncmp(userInput, "getft", 5) == 0) {
                int count = 0;
   		 int offset = 0;
   		 char attribute[255];

    		while (sscanf(userInput + offset, "%s", attribute) == 1) {
       		 count++;
       		 offset += strlen(attribute) + 1;
   		}
   		
   		printf("Number of attributes:: %d\n", count);
   		
   		if(count<1 || count>4){
   		printf("Syntax is incorrect, usage:: getft ext1 ext2 ext3 (max of 3 allowed)\n");
   		
   		}
   		else{
   		send(sockD, userInput, strlen(userInput), 0);
   		}
                
            } else if (strncmp(userInput, "getfdb", 6) == 0) {
                int day, month, year;
  		if (sscanf(userInput, "%*s %d-%d-%d", &year, &month, &day) == 3) {
      			  // Check the range of year, month, and day
        		if (year >= 1000 && year <= 9999 && month >= 1 && month <= 12 && day >= 1 && day <= 31) {
        		 printf("UserInput:: %s\n", userInput);
        		 send(sockD, userInput, strlen(userInput), 0);
       			 }
       			else{
       			 printf("Syntax is incorrect, usage:: getfd YYYY-MM-DD\n");
       		    } 
       			 
   		 } else{
       			 printf("Syntax is incorrect, usage:: getfd YYYY-MM-DD\n");
       		    }
       		    
            } else if (strncmp(userInput, "getfda", 6) == 0) {
                int year, month, day;
  		if (sscanf(userInput, "%*s %d-%d-%d", &year, &month, &day) == 3) {
      			  // Check the range of year, month, and day
        		if (year >= 1000 && year <= 9999 && month >= 1 && month <= 12 && day >= 2 && day <= 31) {
        		 printf("UserInput:: %s\n", userInput);
        		 send(sockD, userInput, strlen(userInput), 0);
       			 }
       			else{
       			 printf("Syntax is incorrect, usage:: getfd YYYY-MM-DD\n");
       		    } 
       			 
   		 } else{
       			 printf("Syntax is incorrect, usage:: getfd YYYY-MM-DD\n");
       		    }
            } else if (strncmp(userInput, "quitc", 5) == 0) {
                send(sockD, userInput, strlen(userInput), 0);
                // Close the socket
    		close(sockD);
    		exit(1);
    		
            } else {
                // Handle invalid command
                printf("Invalid command\nValid commands are listed below\ngetfn filename\ngetfz size1 size2\ngetft <extension list>\ngetfdb date\ngetfda date\n");
            }

            // Receive and print the server's response
            recv(sockD, strData, sizeof(strData), 0);
            printf("Server Response: %s\n", strData);
        }
    }

    return 0;
}
