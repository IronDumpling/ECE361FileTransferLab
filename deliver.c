// Basic
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <time.h>
// Socket
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
// OS
#include <fcntl.h>
// Others
#include "packet.h"

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

int main(int argc, char *argv[])
{
    // PART 1
    // checks for 3 arguments being passed in <deliver> <server address> <port#>
    if (argc != 3) {
        perror("Please enter: <server address> <server port number>\n");
        exit(1);
    }

    // stores arguments passed in to variables
    char* host = argv[1];
    int port = atoi(argv[2]);

    // Step 1. Initialize Server Address Struct
    struct sockaddr_in serverAddr;
    // Clear Memory
    bzero(&serverAddr, sizeof(serverAddr));

    // Initialize Methods
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons((u_short)port);

    // Step 2. Initialize Pointer to the host table entry
    struct hostent* hostAddress;
    hostAddress = gethostbyname(host);
    // Convert Host Name to IP Address
    memcpy(&serverAddr.sin_addr, hostAddress->h_addr_list[0], hostAddress->h_length);

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

    // Set Clock
    clock_t startTime, endTime, startOp, endOp;
    long double initRtt, totalTime, totalOp;

    // Calculate Initial RTT
    // Step 1. Start Timer
    startTime = clock();

    // Step 2. Send an Ack to Server
    socklen_t addressLength = sizeof(struct sockaddr);

    // Send Acknowledge Request
    int snd = sendto(clientSocket, "ftp", strlen("ftp") + 1, MSG_NOSIGNAL,
                     (struct sockaddr*)&serverAddr, addressLength);

    // Send File Failure
    if(snd < 0){
        perror("Send Request Failed");
        exit(1);
    }else{
        printf("Send Request Succeed\n");
    }

    // Step 3. Receive Message from the server
    char recvBuff[PacketSize];

    int length = recvfrom(clientSocket, recvBuff, sizeof(recvBuff), 0,
                          (struct sockaddr*)&serverAddr, &addressLength);

    if(length < 0){
        perror("Receive Message Failed");
        exit(1);
    } else if(strcmp(recvBuff, "yes") == 0){
        printf("A filePtr transfer can start.\n");
    }

    // Step 4. Stop Timer
    endTime = clock();

    // Step 5. Calculate
    totalTime = (float)(endTime - startTime)/CLOCKS_PER_SEC;
    initRtt = totalTime;
    printf("The Round Trip Time is: %Lf second\n", initRtt);

    // PART 2 / 3
    // reads the filePtr in binary mode using the filename passed in
    FILE* filePtr = fopen(fileName, "rb");

    // goes to the end of the filePtr to calculate size of filePtr
    fseek(filePtr, 0, SEEK_END);
    long fileSize = ftell(filePtr);

    // goes back to beginning of filePtr to begin breaking up into packets
    rewind(filePtr);

    // using 1000 as max packet size, find out how many packets are needed for the filePtr, and create data structure for it
    long fragNum = fileSize / DataSize;
    // Remainder
    fragNum += (fileSize % DataSize != 0) ? 1 : 0;

    // creates pointer to store previous packet, store head packet, all to connect linked_list of packets
    char currData[DataSize];
    struct packet *prevPacket, *headPacket, *currPacket;

    // creates linked list adding packets with every iteration
    for (int i = 1; i <= fragNum; i++) {

        // creates a new packet with respective memory allocation
        struct packet *newPacket = malloc(sizeof(struct packet));

        // sets first packet to head, otherwise connect previous packet to this new one
        if (i == 1) {
            headPacket = newPacket;
        }else {
            prevPacket->next = newPacket;
        }

        // stores values for this given packet, reads the first 1000 bytes from filePtr as the data
        newPacket->total_frag = fragNum;
        newPacket->frag_no = i;

        unsigned long n = fread(currData, 1, DataSize, filePtr);
        newPacket->size = n;

        newPacket->filename = fileName;
        memcpy(newPacket->filedata, currData, n);

        newPacket->next = NULL;

        // Changes previous packet pointer to new packet
        prevPacket = newPacket;
    }

    // close filePtr that was being broken into packets
    fclose(filePtr);

    // start at head of the linked list to send to server
    currPacket = headPacket;

    // sets up initial timeout value
    long double devRTT = totalTime;
    long double sampleRTT = totalTime;
    long double timeoutValue = sampleRTT + 4 * devRTT;
    int sentCount;

    // goes through linked list of all packets in order to send each
    while (currPacket != NULL) {
        // Initialization
        char packetSent[PacketSize];
        // sets up counter
        sentCount = 1;
        bool sent = false;

        fd_set readfd;
        FD_ZERO(&readfd);

        struct timeval timeout;

        packetToString(packetSent, currPacket);

        startTime = clock();
        if (currPacket->frag_no == 1) {
            startOp = clock();
        }

        // sends packet to server
        int sd = sendto(clientSocket, "ftp", strlen("ftp") + 1, MSG_NOSIGNAL,
                                 (struct sockaddr*)&serverAddr, addressLength);
        if (sd == -1) {
            perror("Send Packet Failed\n");
            exit(1);
        }

        // repeatedly attempt to send the packet until an ACK has been receieved
        while (!sent) {
            // sets timeout value and reads from the socket for an ACK message
            timeout.tv_sec = timeoutValue / 1;
            timeout.tv_usec = (timeoutValue - timeout.tv_sec) * 1e6;

            FD_SET(clientSocket, &readfd);
            select(clientSocket + 1, &readfd, NULL, NULL, &timeout);

            printf("Packet #%d: ", currPacket->frag_no);

            // the timeout limit reaches
            if (!FD_ISSET(clientSocket, &readfd)) {

                // update the user on the timeout
                // increase the time out value for the subsequent retransmission
                printf("Timeout! ");
                timeoutValue *= 2;

                // attempted to retransmit abort program
                if (sentCount == 20) {
                    endOp = clock();
                    totalOp = (float)(endOp - startOp)/CLOCKS_PER_SEC;
                    printf("Attempted to resend packet #%d 20 times\n "
                           "Aborting filePtr transfer at a time of %Lf seconds.\n",
                           currPacket->frag_no, totalOp);
                    exit(1);
                }
                // otherwise attempt to send again
                else {
                    // sends packet again to the socket
                    int resentPacket = sendto(clientSocket, packetSent, PacketSize, 0,
                                              (struct sockaddr*)&serverAddr, addressLength);

                    if (resentPacket == -1) {
                        perror("Send Packet Failed'\n");
                        exit(1);
                    }

                    sentCount += 1;
                    printf("Resent packet #%d %d times.\n", currPacket->frag_no, sentCount);
                }
            }

            // if time out did not occur, then check what was receieved
            else {
                // receives message from the socket
                int rcv = recvfrom(clientSocket, recvBuff, PacketSize, 0,
                                   (struct sockaddr*)&serverAddr, &addressLength);
                if (rcv == -1) {
                    printf("Error in receiving ACK message from server\n");
                    sent = false;
                }

                endTime = clock();

                // if we receive ack, move on to next packet
                if(strcmp(recvBuff, "ACK") == 0){
                    printf("ACK received for packet #%d.\n", currPacket->frag_no);
                    sent = true;
                }
            }
        }

        // updating to new sample time for the next packet
        sampleRTT = (1 - 0.125) * sampleRTT + (0.125) * (float)(endTime - startTime)/CLOCKS_PER_SEC;
        devRTT = (0.75) * devRTT + (0.25) * fabs(sampleRTT - (float)(endTime - startTime)/CLOCKS_PER_SEC);
        timeoutValue = sampleRTT + 4 * devRTT;

        // moves onto sending next packet, and frees the string for the next iteration
        currPacket = currPacket->next;
    }

    printf("Initial RTT for connection: %Lf.\n", initRtt);

    // frees memory and closes connection
    close(clientSocket);

    return 0;
}
