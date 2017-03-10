#include <stdlib.h>
#include <stdio.h>


typedef enum State STATE;

enum State {
	FILENAME, FILENAMEOK, RECVDATA, ACK, SREJ, BYE, DONE
};

void parseArgs(int, char**);

void initWindow();

STATE filename();