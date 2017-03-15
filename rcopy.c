#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>

#include "rcopy.h"
#include "cpe464.h"
#include "networks.h"

char* localFile;
char* remoteFile;
int32_t windowSize;
int32_t bufferSize;
float errorPercent;
struct in_addr remoteMachine;
int16_t remotePort;
Packet * windowBuffer;
STATE state = START;
Connection server;
int attemptCount = 0;
int sequenceNum = 0;
int32_t localFileDesc = 0;
int32_t rrNumber = 0;
int32_t windowBottom = 0;
int32_t windowNdx = 0;
int32_t windowTop;
int done = 0;

void printVars() {
	printf("GOLBALS-------\n");
	printf("  localFile is       %s\n", localFile);
	printf("  remoteFile is      %s\n", remoteFile);
	printf("  windowSize is      %d\n", windowSize);
	printf("  bufferSize is      %d\n", bufferSize);
	printf("  errorPercent is    %f\n", errorPercent);
	printf("  remoteMachine is   %s\n", inet_ntoa(remoteMachine));
	printf("  remotePort is      %d\n", remotePort);
	printf("GLOBALS--------DONE\n");
}

int main(int argc, char** argv) {
	
	parseArgs(argc, argv);

	//printVars();

	sendtoErr_init(errorPercent, DROP_ON, FLIP_ON, DEBUG_OFF, RSEED_ON);

	initWindow();

	while (state != DONE) {
		switch (state) {
			case START:
				
				printf("\nCASE START\n");
				state = start();
				break;

			case FILENAME:
				
				printf("\nCASE FILENAME\n");
				state = filename();
				break;

			case FILENAMEOK:

				printf("\nCASE FILENAMEOK\n");
				state = filenameok();
				break;

			case RECVDATA:
				printf("\nCASE RECVDATA\n");
				state = recvdata();
				break;

			case ACK:
				
				printf("\nCASE ACK\n");
				state = ack();
				break;

			case SREJ:

				printf("\nCASE SREJ\n");
				srej();
				break;

			case DONE:
				printf("\nCASE DONE\n");
				break;
		}
	}

	printf("DONE\n");	

}


void parseArgs(int argc, char** argv) {
	if (argc != 8) {
		printf("Incorrect usage: ./rcopy local-file remote-file window-size buffer-size error-percent remote-machine remote-port\n");
		exit(-1);
	}
	if ((strlen(argv[1]) > 100) || (strlen(argv[2]) > 100)) {
		printf("File name(s) too long. Must be less than 100 characters");
		exit(-1);
	} else {
		localFile = argv[1];
		remoteFile = argv[2];
	} 
	windowSize = atoi(argv[3]);
	if (windowSize == 0) {
		printf("Incorrect Windows Size\n");
		exit(-1);
	}
	bufferSize = atoi(argv[4]);
	errorPercent = atof(argv[5]);
	inet_aton(argv[6], &remoteMachine);
	remotePort = atoi(argv[7]);

	windowTop = windowSize;
}

void initWindow() {
	windowBuffer = malloc(windowSize * sizeof(Packet));
}

STATE srej() {
	int32_t recv_len = 0;
	uint8_t inbuf[MAX_LEN];
	uint8_t flag;
	int32_t seq_num;

	if (select_call(server.sk_num, 10, 0, NOT_NULL) == 0) {
		printf("Cient timeout\n");
		return DONE;
	}

	recv_len = recv_buf(inbuf, MAX_LEN, server.sk_num, &server, &flag, &seq_num);
	/*printf("SREJ Packet recieved is:\n");
	printf("-->%s\n",(char *) (inbuf));
	printf("--> sequence number %d\n", seq_num);
	printf("--> current window bottom is %d\n", windowBottom);
*/
	if (recv_len == -1) {
		printf("__Srej CRC Error\n");
	}

	if (seq_num == windowBottom) {
		write(localFileDesc, inbuf, bufferSize);
		//printf("+++++Written to buffer > %s\n", inbuf);
		windowBottom++;
		//printf("__Packet with correct seq_num 1\n");
		ack();

		writeValidPackets();


		return RECVDATA;
	}

	if (seq_num > windowBottom) {
		memcpy(windowBuffer[seq_num % windowSize].buffer, inbuf, MAX_LEN);
			windowBuffer[seq_num % windowSize].valid = 1;
	}
	printWindow(windowBuffer, windowSize);

	return SREJ;
}

STATE filename() {

	uint8_t packet[MAX_LEN];
	uint8_t buf[MAX_LEN];

	int32_t nameLen = strlen(remoteFile) + 1;

	memcpy(buf, &windowSize, 4);
	memcpy(buf + 4, remoteFile, nameLen);

	send_buf(buf, nameLen + 4, &server, 1, 0, packet);

	return RECVDATA;
}

STATE start() {
	STATE returnValue = START;
	uint8_t buf[MAX_LEN];
	uint8_t packet[MAX_LEN];
	int32_t buf_size = bufferSize;
	int32_t recv_check = 0;
	uint8_t flag = 0;
	int32_t seq_num = 0;
	
	if (udp_client_setup(inet_ntoa(remoteMachine), remotePort, &server) < 0) {
		perror("Error on udp_client_setup exiting...");
		exit(-1);
	} 
	
	buf_size = htonl(buf_size);

	memcpy(buf, &bufferSize, 4);
	send_buf(buf, 4, &server, 1, 0, packet);

	if (select_call(server.sk_num, 1, 0, NOT_NULL) == 1) {
		recv_check = recv_buf(packet, MAX_LEN, server.sk_num, &server, &flag, &seq_num);

		if(recv_check == -1) {
			return returnValue;
		} else if (flag == 2) {
			returnValue = FILENAME;
		}
	} else {
		close(server.sk_num);
		printf("_socket closed\n");
		attemptCount++;
		printf("_attemptCount is : %d\n",attemptCount );
		if (attemptCount > 9) {
			printf("Server Unreachable, exiting...\n");
			state = DONE;
		}
	}
	return returnValue;
}

STATE filenameok() {
	localFileDesc = open (localFile, O_CREAT | O_TRUNC | O_WRONLY, 0600);
	if (localFileDesc < 1) {
		perror("Error on file open");
		return DONE;
	} else {
		return RECVDATA;
	}
}

STATE ack() {
	int32_t flag = 5;
	uint8_t packet[MAX_LEN];

	send_buf((uint8_t *)&windowBottom, 4, &server, flag, sequenceNum, packet);
	sequenceNum++;
	printPacket(packet);
	if (done) {
		return DONE;
	}
	return RECVDATA;
}

STATE recvdata() {
	int32_t read_len = 0;
	uint8_t * buffer = malloc(MAX_LEN);
	uint8_t flag = 0;
	int32_t seq_num;

	if (select_call(server.sk_num, 10, 0, NOT_NULL) == 1) {
		read_len = recv_buf(buffer, MAX_LEN, server.sk_num, &server, &flag, &seq_num);
		
		if (localFileDesc == 0) {
			localFileDesc = open (localFile, O_CREAT | O_TRUNC | O_WRONLY, 0600);
				if (localFileDesc < 1) {
				perror("Error on file open");
				return DONE;
			}
		}

		if (read_len == -1) { //CRC Error
			windowNdx = windowBottom;
			send_buf((uint8_t *)&windowBottom, MAX_LEN, &server, 6, sequenceNum, buffer);
			printPacket(buffer);
			return SREJ;
		}

		if (flag == 9) { //Bad file name packet
			printf("Bad File Name\n");
			return DONE;
		}

		if (flag == 10) { //EOF Packet
			printf("EOF PACKET RECIEVED...DONE\n");
			done = 1;
			ack();
			return DONE;
		}

		//printf("Recieving seq_num is %d\n", seq_num);
		//printf("windowBottom is %d\n", windowBottom);
		if(seq_num == windowBottom) {
			write(localFileDesc, buffer, read_len);
			//printf("+++++Written to buffer ->%s\n",(char *)buffer );
			windowBottom++;
			ack();
			//printf("__Packet with correct seq_num\n");

			writeValidPackets();
			
			return RECVDATA;
		}

		if (seq_num > windowBottom) {
			memcpy(windowBuffer[seq_num % windowSize].buffer, buffer, MAX_LEN);
			windowBuffer[seq_num % windowSize].valid = 1;
			//printf("__Packet with INCORRECT seq_num\n");
			windowNdx = seq_num;
			ack();
			return RECVDATA;
		}
		

	}
	printf("Server timed out after 10 seconds\n");
	return DONE;

}

void writeValidPackets() {
	int max = windowBottom + windowSize;

	for (windowBottom = windowBottom; windowBottom < max; windowBottom++) {
		if (windowBuffer[windowBottom % windowSize].valid) {
			write(localFileDesc, windowBuffer[windowBottom % windowSize].buffer, bufferSize - 8);
			// printf("+++++Written to buffer -->%s\n",windowBuffer[windowBottom % windowSize].buffer);
			windowBuffer[windowBottom % windowSize].valid = 0;
			ack();
		} else {			
			return;
		}
	}
}