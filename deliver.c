// Basic
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
// Socket
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
// OS
#include <fcntl.h>

// Function 1. Client Function
int main(int argc, char **argv)
{
    // Step 0. Get Input
    char* host = argv[1];
    int port = atoi(argv[2]);

    // Step 1. Initialize Server Address Struct
    struct sockaddr_in serverAddress;
    // Clear Memory
    bzero(&serverAddress, sizeof(serverAddress));

    // Initialize Methods
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons((u_short)port);

    // Step 2. Initialize Pointer to the host table entry
    struct hostent* hostAddress;
    hostAddress = gethostbyname(host);
    // Convert Host Name to IP Address
    memcpy(&serverAddress.sin_addr, hostAddress->h_addr_list[0], hostAddress->h_length);

    // Step 3. Create Client UDP Socket
    int clientSocket = socket(PF_INET, SOCK_DGRAM, 0);
    // If creation fail, Exit
    if(clientSocket < 0){
        perror("Set Up Socket Failed");
        exit(1);
    }

    // Step 4. User Input File Name
    // Output Guidance
    printf("Please Input File Name: ");
    // Get File Name
    char prefix[4], fileName[256];
    scanf("%s %s", prefix, fileName);

    // Step 6. Check if the file exists
    int fd = open(fileName, O_RDWR | O_NOCTTY | O_NDELAY);
    if(fd < 0){
        perror("This File Does Not Exist");
        exit(1);
    }

    // Step 7. Send Message to the server
    char message[] = "ftp";
    socklen_t addressLength = sizeof(struct sockaddr);
    int snd = sendto(clientSocket, message, strlen(message) + 1, MSG_NOSIGNAL,
                      (struct sockaddr*)&serverAddress, addressLength);

    if(snd < 0){
        perror("Send Message Failed");
        exit(1);
    }

    // Step 8. Receive Message from the server
    char buffer[1024];
    int length = recvfrom(clientSocket, buffer, strlen(buffer), 0,
                          (struct sockaddr*)&serverAddress, &addressLength);

    if(length < 0){
        perror("Receive Message Failed");
        exit(1);
    }

    if(strcmp(buffer, "yes") == 0){
        printf("A file transfer can start.\n");
    }

    // Step 9. Close all
    close(clientSocket);
    return 0;
}
