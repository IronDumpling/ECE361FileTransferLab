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
 
 //*************************************************************************************
 //                                     PART 1
 //*************************************************************************************
 
// Check if the usage is correct.

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

 //*************************************************************************************
 //                                     PART 2 / 3
 //*************************************************************************************
 
 // initializes file structure and necessary variables to detect all packets and write to file
 FILE * file;
 char data[PacketSize];
 bool flag = true;
 int count = 1;
 
 // repeats continuously until all packets have been receieved
 while (flag) {
  
  // detects the incoming packet from the client and error checks
  int received_bytes_temp = recvfrom(serverFd, data, PacketSize, 0, (struct sockaddr *)&clientAddr, &addrLen);
  if (received_bytes_temp == -1) {
   printf("Error in receiving the packet message\n");
   return 0;
  }
 
  // creates a packet and fills it with its corresponding members
  struct packet * curr_packet = malloc(sizeof(struct packet));
  stringToPacket(data, curr_packet);
  
  // if the last packet has been receieved, exit the loop
  if (curr_packet->frag_no == curr_packet->total_frag) {
   flag = false;
  }
  
  // if this is the first packet, then open a file so that the incoming data can be written into it
  if (count == 1) {
   file = fopen(curr_packet->filename, "wb");
  }
  
  // only writes to file if the correct packet number is receieved
  if (count == curr_packet->frag_no) {
   
   // writes data to file
   fwrite(curr_packet->filedata, 1, curr_packet->size, file);
   
   // sends back an "ACK" message to acknowledge packet has been receieved, and error checks
   int sending_ack = sendto(serverFd, "ACK", 75, 0, (struct sockaddr *)&clientAddr, addrLen);
   if (sending_ack == -1) {
    printf("Error in sending the 'ACK' message\n");
    return 0;
   }
   
   // increments counter to only write subsequent packet the next iteration
   count++;
  }
  
  // frees the packet struct before moving onto the next one
  free(curr_packet);
 }
 
   // closes file, connection
   fclose(file);
   close(serverFd);
 
   return 0;
}
