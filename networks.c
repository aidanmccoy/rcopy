//Written by Hugh Smith
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "networks.h"
#include "cpe464.h"

int32_t udp_server(int32_t portNumber) {

	int sk = 0; 					//socket descriptor
	struct sockaddr_in local; 		//socket address for us
	uint32_t len = sizeof(local); 	//length of local address
	
	//create the socket
	if ((sk = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
	{
		perror("socket");
		exit(-1);
	}
	
	//set up the socket
	
	local.sin_family = AF_INET; //internet family
	local.sin_addr.s_addr = INADDR_ANY; //wild card machine address
	local.sin_port = htons(portNumber);
	
	//bind the name (address) to a port
	if (bindMod(sk,(struct sockaddr *)&local, sizeof(local)) < 0) {
		perror("udp_server, bind");
		exit(-1);
	}
	
	//get the port name and print32_t it out
	getsockname(sk,(struct sockaddr *)&local, &len);
	printf("Using Port#: %d\n", ntohs(local.sin_port));
	
	return(sk);
}

int32_t select_call(int32_t socket_num, int32_t seconds, int32_t microseconds, int32_t set_null) {
	fd_set fdvar;
	struct timeval aTimeout;
	struct timeval * timeout = NULL;
	
	if (set_null == NOT_NULL) {
		aTimeout.tv_sec = seconds; // set timeout to 1 second
		aTimeout.tv_usec = microseconds; // set timeout (in micro-second)
		timeout = &aTimeout;
	}
	
	FD_ZERO(&fdvar); //reset variables
	FD_SET(socket_num, &fdvar);

	if (select(socket_num+1, (fd_set *) &fdvar, (fd_set *) 0, (fd_set *) 0, timeout) < 0) {
		perror("Select");
		exit(-1);
	}
		
	if (FD_ISSET(socket_num, &fdvar)) 
		return 1;
	else
		return 0;	
}


int32_t recv_buf(uint8_t * buf, int32_t len, int32_t recv_sk_num, Connection * connection, uint8_t * flag, int32_t * seq_num) {
	char data_buf[MAX_LEN];
	int32_t recv_len = 0;
	int32_t dataLen = 0;

	recv_len = safeRecv(recv_sk_num, data_buf, len, connection);

	dataLen = retrieveHeader(data_buf, recv_len, flag, seq_num);

	if(dataLen > 0) {
		memcpy(buf, &data_buf[sizeof(Header)], dataLen);
	}

	return dataLen;
}

int32_t safeRecv(int recv_sk_num, char * data_buf, int len, Connection * connection) {
	int recv_len = 0;
	uint32_t remote_len = sizeof(struct sockaddr_in);

	if ((recv_len = recvfrom(recv_sk_num, data_buf, len, 0, (struct sockaddr *) &(connection->remote), &remote_len)) < 0) {
		perror("recv_buf, recvfrom");
		exit(-1);
	}

	connection->len = remote_len;

	return recv_len;
}

int retrieveHeader(char * data_buf, int recv_len, uint8_t * flag, int32_t * seq_num) {
	Header * aHeader = (Header *)data_buf;
	int returnValue = 0;

	if(in_cksum((unsigned short *) data_buf, recv_len) != 0) {
		returnValue = -1; //CRC Error
	} else {
		*flag = aHeader->flag;
		memcpy(seq_num, &(aHeader->seq_num), sizeof(aHeader->seq_num));
		*seq_num = ntohl(*seq_num);

		returnValue = recv_len - sizeof(Header);
	}

	return returnValue;
}
