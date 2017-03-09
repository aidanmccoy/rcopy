#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <stdint.h>
#include <unistd.h>

#include "server.h"
#include "cpe464.h"
#include "networks.h"


typedef enum State STATE;

enum State {
	START, DONE
};

static float errorPercent;
int32_t portNumber = 0;
pid_t pid;
int32_t serverSK;
uint8_t buffer[MAX_LEN];
Connection client;
int32_t recv_len;
uint8_t flag;
int32_t seq_num;

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
	printf("UDP Server set up...");
	

	while(1) {
		if (select_call(serverSK, 1, 0, NOT_NULL)  == 1) {
			recv_len = recv_buf(buffer, 1000, serverSK, &client, &flag, &seq_num);

			if(recv_len == -1) { //Checksum Error
				printf("Checksum Error, ignore packet\n");
			} else {
				pid = fork();
				if (pid < 0) {
					perror("Error on fork, exiting...");
					exit(-1);
				}
				if (pid == 0) {
					//sendFile(...);
					exit(0);
				}
			}
		}
	}
	return 0;
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