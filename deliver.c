// Basic
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
// Socket
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <libgen.h>
// OS
#include <fcntl.h>
// Others
#include <time.h>
#include "packet.h"

#define ALIVE 32

// Function Prototypes
bool getFile(char*);
void requestACK(int clientSocket, struct sockaddr_in*);
void sendFile(int, struct sockaddr_in*, char*);
void receiveFile(int, struct sockaddr_in*);
void packetToString(char*, struct packet*);
void stringToPacket(char*, struct packet*);

// Function 1. Client Main Function
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
    char fileName[256];
    if(!getFile(fileName)){
        perror("This File Does Not Exist");
        exit(1);
    }

    // Step 5. Send File to the server
    sendFile(clientSocket, &serverAddress, fileName);

    // Step 6. Receive Message from the server
    receiveFile(clientSocket, &serverAddress);

    // Step 7. Close all
    close(clientSocket);
    return 0;
}

// Function 2. Get File
bool getFile(char fileName[]){
    // Output Guidance
    printf("Please Input File Name: ");

    // Get File Name
    char prefix[4];
    scanf("%s %s", prefix, fileName);

    // Check if the file exists
    int fd = open(fileName, O_RDWR | O_NOCTTY | O_NDELAY);
    if(fd < 0){
        return false;
    }
    return true;
}

// Function 3. Send File Function
void sendFile(int clientSocket, struct sockaddr_in *serverAddress, char fileName[]){
    // Step 1. Wave Hand
    // Request Acknowledge
    requestACK(clientSocket, serverAddress);

    // Receive File
    receiveFile(clientSocket, serverAddress);

    // Step 2. Preparation
    // Initialize a File Struct
    FILE *filePtr = fopen(fileName, "rb");

    // Get Size of file
    int fsk = fseek(filePtr, 0, SEEK_END);
    // Error Handling
    if(fsk){
        perror("Seek File Failed");
        exit(1);
    }

    long fileSize = ftell(filePtr);
    // Let filePtr point to the head of the file again
    rewind(filePtr);

    // Step 3. Read File
    // Create a Char Array of the File
    char file[fileSize];
    fread(file, sizeof(char), fileSize, filePtr);
    fclose(filePtr);

    // Get the number of fragments
    long fragNum = fileSize/DataSize;
    // Remainder
    fragNum += (fileSize % DataSize != 0) ? 1 : 0;

    // Send Multiple Fragments
    // Initialize packets
    char **packets = malloc(sizeof(char*) * fragNum);
    char receBuff[DataSize];

    for(int frag = 0; frag < fragNum; ++frag){
        struct packet currPacket;

        // Assign Info
        currPacket.total_frag = fragNum;
        currPacket.frag_no = frag;
        currPacket.size = (frag == (fragNum - 1)) ? fileSize % DataSize : DataSize;
        currPacket.filename = basename(fileName);

        // Copy File
        memcpy(currPacket.filedata, file + (frag * DataSize), currPacket.size);

        // Convert Packet to String
        packets[frag] = malloc(PacketSize * sizeof(char));
        packetToString(packets[frag], &currPacket);
    }

    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    int ssk = setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
    if(ssk < 0){
        perror("Set Socket Failed\n");
        exit(1);
    }

    int timeSent = 0;
    socklen_t addressLength = sizeof(struct sockaddr);

    struct packet ackPacket;
    ackPacket.filename = (char*)malloc(PacketSize * sizeof(char));


    for(int frag = 0; frag < fragNum; ++frag){
        ++timeSent;

        // Send string packet
        int snd = sendto(clientSocket, packets[frag], PacketSize, 0,
                         (struct sockaddr*)serverAddress, addressLength);

        if(snd < 0){
            perror("Send Packet Failed");
            exit(1);
        }else{
            printf("Send Packet %d Succeed\n", frag);
        }

        // Receive ACK
        memset(receBuff, 0, sizeof(char) * DataSize);

        int length = recvfrom(clientSocket, receBuff, DataSize, 0,
                              (struct sockaddr*)serverAddress, &addressLength);

        if(length < 0){
            // Resend if Timeout
            if(timeSent < ALIVE)
                continue;
            // Else exit
            perror("Receive Message Failed");
            exit(1);
        }

        stringToPacket(receBuff, &ackPacket);

        // Check contents of ACK Packet
        if(strcmp(ackPacket.filename, fileName) == 0){
            if(ackPacket.frag_no == frag){
                if(strcmp(ackPacket.filedata, "ACK") == 0){
                    timeSent = 0;
                    continue;
                }
            }
        }

        // Resend
        fprintf(stderr, "ACK packet #%d not received, resending attempt #%d\n", frag, timeSent);
        --frag;
    }

    // Free Mem
    for(int frag = 0; frag < fragNum; ++frag){
        free(packets[frag]);
    }
    free(packets);
    free(ackPacket.filename);
}

// Function 4. Receive File Function
void receiveFile(int clientSocket, struct sockaddr_in* serverAddress){
    // Receive Message from the server
    char buffer[1024];
    socklen_t addressLength = sizeof(struct sockaddr);

    int length = recvfrom(clientSocket, buffer, sizeof(buffer), 0,
                          (struct sockaddr*)serverAddress, &addressLength);

    if(length < 0){
        perror("Receive Message Failed");
        exit(1);
    }

    if(strcmp(buffer, "yes") == 0){
        printf("A file transfer can start.\n");
    }
}

// Function 5. Require Acknowledge Number
void requestACK(int clientSocket, struct sockaddr_in *serverAddress){
    socklen_t addressLength = sizeof(struct sockaddr);

    // Send Acknowledge Request
    int snd = sendto(clientSocket, "ftp", strlen("ftp") + 1, MSG_NOSIGNAL,
                     (struct sockaddr*)serverAddress, addressLength);

    // Send File Failure
    if(snd < 0){
        perror("Send Request Failed");
        exit(1);
    }else{
        printf("Send Request Succeed\n");
    }
}
