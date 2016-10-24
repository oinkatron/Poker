#ifndef CHANNEL_H
#define CHANNEL_H

#include <sys/types.h>
#include <netinet/in.h>

#define MAJOR_VERSION 0
#define MINOR_VERSION 3

#define PCKT_HEAD_SIZE 17

struct Pckt {
	uint64_t sent_t;

	uint8_t flags;
	uint32_t seq_id;

	uint16_t size;
	char *payload;
};

struct RecvPckt {
	uint64_t recv_t;
	uint8_t flags;
	uint32_t seq_id;

	struct sockaddr_in origin;

	uint32_t ack_seq;
	uint32_t ack_bits;

	uint16_t size;
	char *payload;
};

#define REQ_CONNECT 	1
#define REQ_PASS		2
#define REQ_QUERY		3

struct Request {
	uint32_t req_id;
	uint32_t req_type;
};

#define ACK_BUFF_LEN 32
#define REQ_BUFF_LEN 32
struct Channel;

struct HostData {
	struct sockaddr_in r_addr;

	//timing data
	uint16_t timeout_t;

	uint64_t last_rcv_t;
	uint64_t last_snt_t;

	int32_t avg_rtt;
	int32_t total_rtt;

	void (*c_callback)(struct Channel*, int16_t, int8_t);

	//Packet Buffer
	struct Pckt ack_buff[ACK_BUFF_LEN];
	int32_t ack_cnt;

	//Request Buffer
	//struct Request req_buff[REQ_BUFF_LEN];
	//int32_t req_cnt;
	
	//Ack data
	uint32_t l_seq;

	uint32_t ack_seq;
	uint32_t ack_bits;

	//analytics
	uint32_t n_recvd;
	uint32_t n_sent;
	uint32_t n_acked;
};

struct HostData* Host_Create(struct sockaddr_in *s);
void Host_Free(struct HostData *hd);

struct HostBuff {
	struct HostData* hosts;
	uint16_t size;
	uint16_t cap;
};

struct HostBuff* HBuff_Create(uint16_t len);
void HBuff_Init(struct HostBuff *hb, uint16_t len);
void HBuff_Free(struct HostBuff* hb);

struct HostData *HBuff_Push(struct HostBuff *hb, struct HostData *hd);
struct HostData *HBuff_Get(struct HostBuff *hb, struct sockaddr_in *ind);
int32_t HBuff_Remove(struct HostBuff *hb, struct sockaddr_in *ind);
int32_t HBuff_RemoveByIndex(struct HostBuff *hb, int16_t ind);

#define PCKT_FLAG_CONNECT 0x01
#define PCKT_FLAG_CONNACK 0x02
#define PCKT_FLAG_DISCONNECT 0x04
#define PCKT_FLAG_QUERY 0x08
#define PCKT_FLAG_QUERYACK 0x10
#define PCKT_FLAG_PING 0x20
#define PCKT_FLAG_MSG 0x40

struct Channel {
	int sock;
	struct sockaddr_in l_addr;

	//buffers and size
	uint16_t max_msg_size;
	char* send_buff;
	uint16_t send_size;
	char *recv_buff;
	uint16_t recv_size;

	//timing stuff
	uint64_t host_timeout;

	uint64_t start_time;	
	uint64_t cur_time;

	struct HostBuff hosts;
	struct HostBuff pending;

	////////////////////
	//callback functions
	////////////////////
	//void (*on_connect)(struct Channel *ch, struct RecvPckt *p); //after connection completes (both ends)
	void (*on_reject)(struct Channel *ch, struct RecvPckt *p); //after connection is rejected (end thats being connected too)
	void (*on_accept)(struct Channel *ch, struct RecvPckt *p); //when you accept a connection
	void (*on_query)(struct Channel *ch, struct RecvPckt *p); //after recieving a query, sends response
   void (*on_query_response)(struct Channel *ch, struct RecvPckt *p);
	void (*on_msg)(struct Channel *ch, int16_t host_id, struct RecvPckt *p); //after recieving a msg
	void (*on_disconnect)(struct Channel *ch, int16_t host_id); //both ends, on timeout or when disconnct is send/recieved
	void (*on_ping)(struct Channel *ch, int16_t host_id); //When you recieve a ping
	void (*on_drop)(struct Channel *ch, int16_t host_id, struct Pckt *p); //when a packet gets dropped
	void (*on_quit)(struct Channel *ch);
};

struct Channel*	Channel_Open(const char *addr_str, uint16_t max_hosts, uint16_t m_len, uint16_t to);
void 					Channel_Close(struct Channel *ch); //Removes all hosts

void 					Channel_Quit(struct Channel *ch, int q_code);

void 					Channel_Connect(struct Channel *ch, const char *addr_str, void (*cb)(struct Channel*, int16_t h_id, int8_t));
void 					Channel_Disconnect(struct Channel *ch, int16_t h_id);

int32_t	Channel_QueryHost(struct Channel *ch,
					 						 	const char *addr_str, char q_type);

int32_t				Channel_Recv(struct Channel *ch,
					 					 int32_t timeout);
int32_t 				Channel_Send(struct Channel *ch,
					 					 struct HostData *hd,
										 uint8_t f,
										 const char *buff, 
										 uint16_t buff_len);

int32_t           Channel_SendRaw(struct Channel *ch, struct sockaddr_in *ra, char flags, char *msg, int16_t msg_len);

void 					Channel_Update(struct Channel *ch,
					 						uint64_t ct);

void 					Channel_ProcessPckt(struct Channel *ch, 
					 			 				  struct HostData *hd, 
								 				  struct RecvPckt *p);

int16_t 				Channel_CreatePending(struct Channel *ch, 
					 								 struct sockaddr_in *addr);
int16_t 				Channel_CreateHost(struct Channel *ch, 
					 							 struct sockaddr_in *addr);

void Channel_RemovePending(struct Channel *ch, int16_t ind);
void Channel_RemoveHost(struct Channel *ch, int16_t ind);

int16_t Channel_ConfirmPending(struct Channel *ch, int16_t ind);

int16_t Channel_GetPending(struct Channel *ch, struct sockaddr_in *addr);
int16_t Channel_GetHost(struct Channel *ch, struct sockaddr_in *addr);
#endif
