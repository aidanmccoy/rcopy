#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "server.h"
#include "cpe464.h"
#include "networks.h"




static float errorPercent;
int32_t portNumber = 0;
pid_t pid;
int32_t serverSK;
uint8_t buffer[MAX_LEN];
Connection client;
int32_t recv_len;
uint8_t flag;
int32_t seq_num;
int status;
int32_t buf_size = 1500;
int32_t remoteFile;
int32_t windowSize;
int32_t windowBottom = 0;
int32_t windowTop = 0;
int32_t windowNdx = 0;
uint8_t * windowBuf;
int32_t serverSeq = 0;


void printGlobals() {
	printf("GLOBALS------\n");
	printf("  errorPercent is  %f\n", errorPercent);
	printf("  portNumber is    %d\n", portNumber);
	printf("GLOBALS------DONE\n");
}

int main(int argc, char *argv[])
{
	parseArgs(argc, argv);
	printGlobals();

	sendtoErr_init(errorPercent, DROP_ON, FLIP_ON, DEBUG_ON, RSEED_ON);

	serverSK = udp_server(portNumber);
	printf("UDP Server set up...\n");
	

	while(1) {
		if (select_call(serverSK, 1, 0, NOT_NULL)  == 1) {
			recv_len = recv_buf(buffer, buf_size, serverSK, &client, &flag, &seq_num);

			if(recv_len == -1) { //Checksum Error
				printf("Checksum Error, ignore packet\n");
			} else {
				pid = fork();
				if (pid < 0) {
					perror("Error on fork, exiting...");
					exit(-1);
				}
				if (pid == 0) {
					process();
					exit(0);
				}
			}
			while (waitpid(-1, &status, WNOHANG) > 0) {
				printf("Child completed... with status %d\n", status);
			}
		}
	}
	return 0;
}

void process() {
	STATE state = START;

	while (state != DONE) {
		switch (state) {
			case START:

				printf("\nCASE: START\n");
				state = start();
				break;

			case FILENAME:

				printf("\nCASE: FILENAME\n");
				state = filename();
				break;

			case SENDPACKET:

				printf("\nCASE: SENDPACKET\n");
				state = sendpacket();
				break;

			case PROCESSPACKET:
				break;
			case ACK:
				break;
			case BYE:
				break;
			case DONE:
				printf("CASE DONE\n");
				exit(0);
				break;

		}
	}
}

STATE sendpacket() {
	int32_t len_read = 0;
	uint8_t * buf = malloc(buf_size);

	if (windowNdx == windowTop) {
		printf("__send packet has reached window limit and needs ack\n");
		return ACK;
	} else {
		len_read = read(remoteFile, buf, buf_size - 8);

		switch (len_read) {
			case -1:
				perror("send_data, read error");
				return DONE;
				break;
			case 0:
				send_buf(buf, 1, &client, 10, serverSeq, buffer); //EOF packet
				printf("__sent eof packet \n");
				printPacket(buffer);
				return ACK;
				break;
			default:
				send_buf(buf, len_read, &client, 3, serverSeq, buffer); //Data packet
				printf("__Sent data packet\n");
				serverSeq++;
				printPacket(buffer);
				windowNdx++;


		}
	}
	/*if (select_call(client.sk_num, 0, 0, NOT_NULL) == 1) {
		return PROCESSPACKET;
	} */
	return SENDPACKET;
}

void parseArgs(int argc, char** argv) {
		if (argc == 2) {
			errorPercent = atof(argv[1]);
		} else if (argc == 3) {
			errorPercent = atof(argv[1]);
			portNumber = atoi(argv[2]);
		} else {
			printf("Invalid number of args\n");
			exit(-1);
		}
}

STATE start() {
	memcpy(&buf_size, buffer, 4);
	uint8_t response[1];
	if((client.sk_num = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("Start, open socket");
		exit(-1);
	}
	printf("_buf_size is %d\n",buf_size );
	send_buf(response, buf_size, &client, 2, 0, buffer);
	printPacket(buffer);

	return FILENAME;

}

STATE filename() {
	char * fileName = malloc(100);
	uint8_t response[1];
	// if (select_call(client.sk_num, 1, 0, NOT_NULL) == 1) {
		printf("___inside selectcall\n");
		recv_len = recv_buf(buffer, MAX_LEN, client.sk_num, &client, &flag, &seq_num);
		printf("___buffer recieved\n");
		printf("___size of Header is %d\n",(int)sizeof(Header) );

		memcpy(&windowSize, buffer, 4);
		memcpy(fileName, buffer + 4, recv_len - 4);
		
		printf("__filename is %s\n", fileName);
		printf("__windowSize is %d\n", windowSize);

		windowTop = windowSize;
		windowBuf = malloc(MAX_LEN * windowSize);

		if ((remoteFile = open(fileName, O_RDONLY)) < 0) {
			send_buf(response, 0, &client, 9, 0, buffer); //Bad file name
			printf("__Sent bad flag 9 packed, bad file name\n");
			return BYE;
		} else {
			printf("__Good file name, file opened\n");
			return SENDPACKET;
		}
	//} else
	return FILENAME;
}			