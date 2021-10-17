#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stdbool.h>
#include <netinet/in.h>
#include "packet.h"

int main(int argc, char* argv[])
{
    // Check if the usage is correct.
    if (argc != 2) {
        printf("Usage: ./server <UDP port number>\n");
        return -1;
    }

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

    // Start to wait for the real file trnasfer
    while (true) {
        char recvData[PacketSize] = "";

        recvfrom(serverFd, recvData, PacketSize, 0, (struct sockaddr*)&clientAddr, &addrLen);

        struct packet packetFrag = {0, 0, 0, NULL};
        memset(packetFrag.filedata, 0, DataSize);

        stringToPacket(recvData, &packetFrag);

        int totalFragNum = packetFrag.total_frag;

        char fileName[strlen(packetFrag.filename) + 10];
        strcpy(fileName, "dest/");
        strcat(fileName, packetFrag.filename);

        FILE* newFile = fopen(fileName, "w+");
        fwrite(packetFrag.filedata, packetFrag.size, 1, newFile);
        fclose(newFile);

        printf("Received Fragment NO.%d; Data Size: %d; packet Size: %ld.\n", packetFrag.frag_no, packetFrag.size, sizeof(packetFrag));

        if (packetFrag.frag_no == (packetFrag.total_frag - 1)) break;

        for (int i = 1; i <= totalFragNum; ++i) {
            memset(recvData, 0, PacketSize);

            packetFrag.total_frag = 0;
            packetFrag.frag_no = 0;
            packetFrag.size = 0;
            free(packetFrag.filename);
            packetFrag.filename = NULL;
            memset(packetFrag.filedata, 0, DataSize);

            printf("Before recv.\n");

            recvfrom(serverFd, recvData, PacketSize, 0, (struct sockaddr*) &clientAddr, &addrLen);

            printf("After recv.\n");

            stringToPacket(recvData, &packetFrag);

            FILE* microNewFile = fopen(fileName, "a");
            fwrite(packetFrag.filedata, packetFrag.size, 1, microNewFile);
            fclose(microNewFile);

            printf("Received Fragment NO.%d; Data Size: %d; packet Size: %ld.\n", packetFrag.frag_no, packetFrag.size, sizeof(packetFrag));

            if (packetFrag.frag_no == (packetFrag.total_frag - 1)) break;
        }

        break;
    }

    printf("The file has been stored\n");

    return 0;
}
