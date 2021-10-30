#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "packet.h"

int main(int argc, char *argv[])
{
    // Check if the usage is correct.
    if (argc != 2) {
        printf("Usage: ./server <UDP port number>\n");
        return -1;
    }
    
    // Part 1

    // Declare a structure to hold IP address for both server and client
    struct sockaddr_in serverAddr;
    struct sockaddr_in clientAddr;
    socklen_t addrLen = sizeof(struct sockaddr);

    // Declare the socket descriptor of the server
    int serverFd;

    // Cast the input port number from string to integer
    int port = atoi(argv[1]);

    // Create a socket at the port and bind a local address to it
    serverFd = socket(PF_INET, SOCK_DGRAM, 0);

    // Clear and initialize the socket address structure
    memset((char*)&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons((u_short)port);

    // Bind the address information to the local pocket
    int validBind = bind(serverFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    if (validBind == 0) {
        printf("Successfully bind the port %d.\n", port);
    } else {
        printf("Failed to bind the port %d.\n", port);
        return false;
    }

    // Wait to hear from the client to check connection validity
    while (true) {
        // Content from the client end & content to be sent back
        char recvData[256];
        char sendData[256];
        // Receive data from clients
        ssize_t preRecvValid = recvfrom(serverFd, recvData, sizeof(recvData), 0, (struct sockaddr*)&clientAddr, &addrLen);
        if (preRecvValid != -1) {
            printf("Successfully receive the foretelling message.\n");
        } else {
            printf("Failed to receive the foretelling message.\n");
            return false;
        }
        // Process the data from clients and give suitable response
        if (strcmp(recvData, "ftp") == 0) {
            strncpy(sendData, "yes", 4);
        } else {
            strncpy(sendData, "no", 3);
        }
        // Send back the response to indicate that a file transfer can start
        ssize_t preSendValid = sendto(serverFd, sendData, strlen(sendData)+1, 0, (struct sockaddr*)&clientAddr, addrLen);
        if (preSendValid != -1) {
            printf("Successfully send the response: %d.\n", strcmp(recvData, "ftp") == 0);
        } else {
            printf("Failed to send the response.\n");
            return false;
        }
        break;
    }

    // Part 2 & 3
    
    // Declaration and initialization
    FILE * file;
    char data[PacketSize];
    bool flag = true;
    int count = 0;
    // Repeats until all packets are received
    while (flag) {
        // increment counter to record the expected fragment number
        count++;
        // clear data buffer
        memset(data, 0, PacketSize);
        // detects the incoming packet from the client and error checks
        int check_recv = recvfrom(serverFd, data, PacketSize, 0, (struct sockaddr *)&clientAddr, &addrLen);
        if (check_recv == -1) {
            printf("Failed to receive packet.\n");
            return 0;
        }
        // create current packet
        struct packet* curr_packet = malloc(sizeof(struct packet));
        stringToPacket(data, curr_packet);
        // if the packet number is not what we are expecting, skip everything and waiting for another message
        if (curr_packet->frag_no != count) continue;
        // exit the loop after writing if all have been received.
        if (curr_packet->frag_no == curr_packet->total_frag) flag = false;
        // if this is the first packet, open a new file and record the file flow as "file"
        if (count == 1) {
            // cast the file name
            char fileName[strlen(curr_packet->filename) + 10];
            strcpy(fileName, "dest/");
            strcat(fileName, curr_packet->filename);
            file = fopen(fileName, "wb");
        }
        // write data to file
        fwrite(curr_packet->filedata, 1, curr_packet->size, file);
        // send back an "ACK" message
        int check_sendACK = sendto(serverFd, "ACK", 75, 0, (struct sockaddr *)&clientAddr, addrLen);
        if (check_sendACK == -1) {
            printf("Failed to send ACK.\n");
            return 0;
        }
        // free the dynamically allocated packet struct
        free(curr_packet);
    }
    // close file and socket
    fclose(file);
    close(serverFd);
    printf("File has been successfully received and stored in directory: './dest'.\n");
    return 0;
}
