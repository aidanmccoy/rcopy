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

void printWindow(Packet * window, int win_size) {
	int ndx;
	printf("------------Window------------------\n");
	for (ndx = 0; ndx < win_size; ndx++) {
		printf("\nWindow location -->%d\n", ndx);
		printf("Data is -->%s\n", (char *)window[ndx].buffer);
		printf("Valid flag is -->%d\n", window[ndx].valid);
	}
	printf("------------------------------------\n");
}

void printPacket(uint8_t * packet) {
	int flag = *(packet + 6); 
	printf("PACKET DATA================\n");
	printf("  SEQ_NUM is        %d\n",  ntohl(*(uint32_t *)packet));
	printf("  Checksum is       %d\n", *(uint16_t *)(packet + 4));
	printf("  Flag is           %d\n", *(uint8_t *)(packet + 6));

	if (flag == 1) {
	printf("  Buf_size is       %d\n", *(uint32_t *)(packet + 8));
	}

	if (flag == 3) {
	printf("  Data is           %s\n", (char *)(packet + 8));
	}

	if (flag == 6) {
	printf("  SREJ packet # is  %d\n", *(uint32_t *)(packet + 8));
	}


	printf("PACKET DATA================DONE\n");
}

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

int32_t udp_client_setup(char * hostname, uint16_t port_num, Connection * connection) {
	struct hostent * hp = NULL; 

	connection->sk_num = 0;
	connection->len = sizeof(struct sockaddr_in);

	if ((connection->sk_num = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("udp_client_setup , socket");
		exit(-1);
	}

	connection->remote.sin_family = AF_INET;

	hp = gethostbyname(hostname);

	if (hp == NULL) {
		printf("Host not found: %s\n", hostname);
		return -1;
	}

	memcpy(&(connection->remote.sin_addr), hp->h_addr,hp->h_length);

	connection->remote.sin_port = htons(port_num);

	return 0;
}

int32_t select_call(int32_t socket_num, int32_t seconds, int32_t microseconds, int32_t set_null) {
	fd_set fdvar;
	struct timeval aTimeout;
	struct timeval * timeout = NULL;
	
	if (set_null == NOT_NULL) {
		aTimeout.tv_sec = seconds;
		aTimeout.tv_usec = microseconds;
		timeout = &aTimeout;
		//printf("TIMEOUT Set\n");
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

	//printf("___recv_len vars declared\n");

	recv_len = safeRecv(recv_sk_num, data_buf, len, connection);
	//printf("___recv_len is ->%d<-\n", recv_len);

	dataLen = retrieveHeader(data_buf, recv_len, flag, seq_num);

	if(dataLen > 0) {
		memcpy(buf, &data_buf[sizeof(Header)], dataLen);
	}

	return dataLen;
}

int32_t safeRecv(int recv_sk_num, char * data_buf, int len, Connection * connection) {
	int recv_len = 0;
	uint32_t remote_len = sizeof(struct sockaddr_in);

	//printf("____safeRecv Vars declared\n");

	if ((recv_len = recvfrom(recv_sk_num, data_buf, len, 0, (struct sockaddr *) &(connection->remote), &remote_len)) < 0) {
		perror("recv_buf, recvfrom");
		exit(-1);
	}
	//printf("____safeRecv recv len is ->%d<-\n", recv_len);

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

int32_t send_buf(uint8_t * buf, uint32_t len, Connection * connection, uint8_t flag, uint32_t seq_num, uint8_t *packet) {
	int32_t sentLen = 0;
	int32_t sendingLen = 0;

	if(len > 0) {
		memcpy(&packet[sizeof(Header)], buf, len);
	}

	sendingLen = createHeader(len, flag, seq_num, packet);

	sentLen = safeSend(packet, sendingLen, connection);

	return sentLen;
}

int createHeader(uint32_t len, uint8_t flag, uint32_t seq_num, uint8_t * packet) {
	Header * aHeader = (Header *) packet;
	uint16_t checksum = 0;

	seq_num = htonl(seq_num);
	memcpy(&(aHeader->seq_num), &seq_num, sizeof(seq_num));

	aHeader->flag = flag;

	memset(&(aHeader->checksum), 0, sizeof(checksum));
	checksum = in_cksum((unsigned short *)packet, len + sizeof(Header));
	memcpy(&(aHeader->checksum), &checksum, sizeof(checksum));

	return len + sizeof(Header);
}

int32_t safeSend(uint8_t * packet, uint32_t len, Connection * connection) {
	int send_len = 0;

	if ((send_len = sendtoErr(connection->sk_num, packet, len, 0, (struct sockaddr *) &(connection->remote), connection->len)) < 0) {
		perror("error in send_buf, send to call");
		exit(-1);
	}

	return send_len;
}