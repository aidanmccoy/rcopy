#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <stdint.h>


#include "rcopy.h"
#include "cpe464.h"


static char* localFile;
static char* remoteFile;
static int32_t windowSize;
static int32_t bufferSize;
static float errorPercent;
static struct in_addr remoteMachine;
static int32_t remotePort;

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