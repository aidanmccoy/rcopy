#include <stdlib.h>
#include <stdio.h>


typedef enum State STATE;

enum State {
	START, FILENAME, FILENAMEOK, RECVDATA, ACK, SREJ, DONE
};

void parseArgs(int, char**);

void initWindow();

STATE filename();

STATE start();

STATE filenameok();

STATE recvdata();

STATE ack();

void writeValidPackets();