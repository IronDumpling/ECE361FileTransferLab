#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>

int main(int argc, char* argv[])
{
    if (argc != 2) {
        printf("Usage: ./server <UDP port number>\n");
        return -1;
    }

    // Declare a structure to hold IP address for both server and client
    struct sockaddr_in serverAddr;
    struct sockaddr_in clientAddr;

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

    bind(serverFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr));

    // Wait to hear from the client
    while (1) {
        // Content from the client end & content to be sent back
        char recvData[256];
        char sendData[256];
        socklen_t addrLen = sizeof(struct sockaddr);

        // Receive data from clients
        recvfrom(serverFd, recvData, sizeof(recvData), 0, (struct sockaddr*)&clientAddr, &addrLen);

        // Process the data from clients and give suitable response
        if (strcmp(recvData, "ftp") == 0)
            strncpy(sendData, "yes", 4);
        else
            strncpy(sendData, "no", 3);

        // Send back the response
        sendto(serverFd, sendData, strlen(sendData)+1, 0, (struct sockaddr*)&clientAddr, addrLen);
    }

    return 0;
}
