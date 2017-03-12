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
int32_t localFileDesc;


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

	printVars();

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
				break;
			case SREJ:
				break;
			case BYE:
				printf("\nCASE BYE\n");
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
}

void initWindow() {
	windowBuffer = malloc(sizeof(WindowData) * windowSize);
}

STATE filename() {
	printf("_filename call\n");

	STATE returnValue = FILENAME;
	uint8_t packet[MAX_LEN];
	uint8_t buf[MAX_LEN];
	uint8_t flag = 0;
	int32_t seq_num = 0;
	int32_t nameLen = strlen(remoteFile) + 1;
	int32_t recv_check = 0;
	//static int retryCount = 0;
	//int32_t buf_size = bufferSize;
	
	//int32_t win_size = htonl(windowSize);

	memcpy(buf, &windowSize, 4);
	memcpy(buf + 4, remoteFile, nameLen);

	send_buf(buf, nameLen + 4, &server, 1, 0, packet);
	printf("__filename() send buf\n");

	if (select_call(server.sk_num, 1, 0, NOT_NULL) == 1) {
		printf("__in filename select call\n");
		recv_check = recv_buf(packet, MAX_LEN, server.sk_num, &server, &flag, &seq_num);

		if (recv_check == -1) {
			return returnValue;
		}
		if (flag == 9) {//Bad file name
			printf("__Bad file name\n");
			returnValue = BYE;
		} 
		else if (flag == 3) { //Good file name because is data packet
			printf("__good file name\n");
			returnValue = FILENAMEOK;
		}
	}

	return returnValue;
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
	printPacket(packet);
	printf("__start() send buf\n");

	if (select_call(server.sk_num, 1, 0, NOT_NULL) == 1) {
		recv_check = recv_buf(packet, MAX_LEN, server.sk_num, &server, &flag, &seq_num);
		printf("__recieved buff from server with flag %d\n", flag);

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

STATE recvdata() {
	//int32_t read_len = 0;
	uint8_t * buffer = malloc(MAX_LEN);
	uint8_t flag = 0;
	int32_t seq_num;

	printf("Before select call\n");
	if (select_call(server.sk_num, 10, 0, NOT_NULL) == 1) {
		recv_buf(buffer, MAX_LEN, server.sk_num, &server, &flag, &seq_num);
		return RECVDATA;

	} else {
		printf("Server timed out after 10 seconds\n");
		return DONE;
	}

	


}