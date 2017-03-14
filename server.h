#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <stdint.h>


typedef enum State STATE;

enum State {
	START, FILENAME, SENDPACKET, PROCESSPACKET, ACK, BYE, DONE
};

void process();

void parseArgs(int, char**);

void sendFile();

STATE start();

STATE filename();

STATE sendpacket();

STATE processpacket();

STATE ack();

void initWindow();