#define MAX_LEN 1500

enum SELECT{
	SET_NULL, NOT_NULL
};

typedef struct connection Connection;

typedef struct header Header;

typedef struct packet Packet;

typedef struct windowData WindowData;

struct header{
	uint32_t seq_num;
	uint16_t checksum;
	uint8_t flag;
};

struct windowData {
	Packet * buffer;
	int used;
};


struct packet {
	uint32_t seq_num;
	uint16_t checksum;
	uint8_t flag;
	uint32_t packetSize;
	uint8_t payLoad[MAX_LEN - 12];
};

struct connection {
	int32_t sk_num;
	struct sockaddr_in remote;
	uint32_t len;
};

/*Packet * createPacket(uint32_t seq_num, uint8_t flag, uint32_t packetSize, )
*/
int32_t udp_server(int32_t portNumber);

int32_t udp_client_setup(char* hostname, uint16_t port_num, Connection *connection);

int32_t select_call(int32_t socket_num, int32_t seconds, int32_t microseconds, int32_t set_null);

int32_t recv_buf(uint8_t * buf, int32_t len, int32_t recv_sk_num, Connection * connection, uint8_t * flag, int32_t * seq_num);

int32_t safeRecv(int recv_sk_num, char * data_buf, int len, Connection * connection);

int retrieveHeader(char * data_buf, int recv_len, uint8_t * flag, int32_t * seq_num);

int32_t send_buf(uint8_t * buf, uint32_t len, Connection * connection, uint8_t flag, uint32_t seq_num, uint8_t *packet);

int createHeader(uint32_t len, uint8_t flag, uint32_t seq_num, uint8_t * packet);

int32_t safeSend(uint8_t * packet, uint32_t len, Connection * connection);




