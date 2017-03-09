#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <stdint.h>

#include "server.h"
#include "cpe464.h"

static float errorPercent;
static int32_t portNumber;
static pid_t pid;

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
	printf("DONE\n");
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