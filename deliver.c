// Basic
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
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
	//                                     PART 1
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
    struct timeval startTime, endTime, startOp, endOp;
    long double initRtt, totalTime, totalOp;

    // Start Timer
    gettimeofday(&startTime, 0);

    // Send an Ack to Server
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

    // Receive Message from the server
    char recvBuff[PacketSize];

    int length = recvfrom(clientSocket, recvBuff, sizeof(recvBuff), 0,
                          (struct sockaddr*)&serverAddr, &addressLength);

    if(length < 0){
        perror("Receive Message Failed");
        exit(1);
    }

    if(strcmp(recvBuff, "yes") == 0){
        printf("A file transfer can start.\n");
    }

    // Stop Timer
    gettimeofday(&endTime, 0);

    long double seconds = endTime.tv_sec - startTime.tv_sec;
    long double microSec = endTime.tv_usec - startTime.tv_usec;

    totalTime = seconds + microSec * 1e-6;
    initRtt = totalTime;

    printf("The Round Trip Time is: %Lf second\n", initRtt);

	//                                     PART 2 & 3
	// reads the file in binary mode using the filename passed in
	FILE* file = fopen(fileName, "rb");
	
	// goes to the end of the file to calculate size of file
	fseek(file, 0, SEEK_END);
	int f_size = ftell(file);
	
	// goes back to beginning of file to begin breaking up into packets
	fseek(file, 0, SEEK_SET);
	
	// using 1000 as max packet size, find out how many packets are needed for the file, and create data structure for it
	int num_of_fragments = (f_size/DataSize) + 1;
	char data_for_packet[DataSize];

	// creates pointer to store previous packet, store head packet, all to connect linked_list of packets
	struct packet *prev_packet, *head_packet, *curr_packet;
	
	// creates linked list adding packets with every iteration
	for (int i = 1; i <= num_of_fragments; i++) {
		
		// creates a new packet with respective memory allocation
		struct packet *new_packet = malloc(sizeof(struct packet));
		
		// sets first packet to head, otherwise connect previous packet to this new one
		if (i == 1) {
			head_packet = new_packet;
		}
		else {
			prev_packet->next = new_packet;
		}
		
		// stores values for this given packet, reads the first 1000 bytes from file as the data
		new_packet->total_frag = num_of_fragments;
		new_packet->frag_no = i;
		int n = fread(data_for_packet, 1, DataSize, file);
		new_packet->size = n;
		new_packet->filename = fileName;
		memcpy(new_packet->filedata, data_for_packet, n);
		new_packet->next = NULL;
		
		// Changes previous packet pointer to new packet
		prev_packet = new_packet;
	}
	
	// close file that was being broken into packets
	fclose(file);
	
	// start at head of the linked list to send to server, initialize a string to read ACK messages from server
	curr_packet = head_packet;
	int sent_count;

	// sets up initial timeout value
	long double devRTT = totalTime;
	long double sampleRTT = totalTime;
	long double timeout_value = sampleRTT + 4*devRTT;
	
	// goes through linked list of all packets in order to send each, and receieve corresponding ACK messages
	while (curr_packet != NULL) {
        char packet_to_send[PacketSize];
        packetToString(packet_to_send, curr_packet);

        gettimeofday(&startTime, 0);
		if (curr_packet->frag_no == 1) {
            gettimeofday(&startOp, 0);
		}

		// sends packet to server
		int sent_packet = sendto(clientSocket, packet_to_send, PacketSize, 0,
                                (struct sockaddr*)&serverAddr, addressLength);

		if (sent_packet == -1) {
			perror("Error in sending packet'\n");
			exit(1);
		}
			
		// sets up counter, flags and variables for the loop to send the packet
		sent_count = 1;
		// bool timeout_flag = false;
		bool sent = false;
		fd_set readfd;
		FD_ZERO(&readfd);
		struct timeval timeout;

		// repeatedly attempt to send the packet until an ACK has been receieved
		while (!sent) {
			
			printf("Packet #%d: ", curr_packet->frag_no);
			
			// sets timeout value and reads from the socket for an ACK message
			timeout.tv_sec = timeout_value/1;
			timeout.tv_usec = (timeout_value - timeout.tv_sec)*1e6;

			FD_SET(clientSocket, &readfd);
			select(clientSocket + 1, &readfd, NULL, NULL, &timeout);
			
			// if the timeout limit is reaches, then display it to the user and attempt to send again if the max limit has not been reaches
			if (!FD_ISSET(clientSocket, &readfd)) {
				
				// update the user on the timeout and increase the time out value for the subsequent retransmission
				printf("Timeout occured. ");
				timeout_value = timeout_value * 2;

				// if attempted to retransmit 18 times, abort program, otherwise attempt to send again
				if (sent_count == 20) {
                    gettimeofday(&endOp, 0);

                    seconds = endOp.tv_sec - startOp.tv_sec;
                    microSec = endOp.tv_usec - startOp.tv_usec;
                    totalOp = (float) seconds + microSec * 1e-6;

                    printf("\nAttempted to resend packet #%d, 20 times\n Aborting file transfer at a time of %Lf seconds.\n", curr_packet->frag_no, totalOp);
					sent_count = 0;
					return 0;
				}
				else {
					// sends packet again to the socket, error  cheks and incrememnts counter for num of times sent
					int sent_packet_again = sendto(clientSocket, packet_to_send, PacketSize, 0, (struct sockaddr*)&serverAddr, addressLength);
					if (sent_packet_again == -1) {
						perror("Error in sending packet'\n");
						exit(1);
					}
					sent_count = sent_count + 1;
					printf("Resent packet #%d, %d times.\n", curr_packet->frag_no, sent_count);
				}
			}
			
			// if time out did not occur, then check what was receieved
			else {
				// receieves message from the socket
				int rcv = recvfrom(clientSocket, recvBuff, PacketSize, 0,
                                  (struct sockaddr*)&serverAddr, &addressLength);
                gettimeofday(&endTime, 0);
				if (rcv == -1) {
					printf("Error in receiving ACK message from server\n");
					sent = false;
				}
				
				// if we receieve ack, move on to next packet
				if(strcmp(recvBuff, "ACK") == 0){
					printf("ACK receieved for packet #%d.\n",curr_packet->frag_no);
					sent = true;
				}
			}
		}
		
		// updating to new sample time for the next packet
        seconds = endOp.tv_sec - startOp.tv_sec;
        microSec = endOp.tv_usec - startOp.tv_usec;
        totalOp = (float) seconds + microSec * 1e-6;

		sampleRTT = (1 - 0.125) * sampleRTT + (0.125) * totalOp;
		devRTT = (0.75) * devRTT + (0.25) * fabs(sampleRTT - totalOp);
		timeout_value = sampleRTT + 4 * devRTT;
		
		// moves onto sending next packet, and frees the string for the next iteration
		curr_packet = curr_packet->next;
	}
	
	// calculates time for the entire transmission
    gettimeofday(&endOp, 0);

	seconds = endOp.tv_sec - startOp.tv_sec;
	microSec = endOp.tv_usec - startOp.tv_usec;
	totalOp = (float) seconds + microSec * 1e-6;

    printf("Initial round trip time for connection: %Lf \n", initRtt);

	// frees memory and closes connection
    close(clientSocket);
	
	return 0;
}
