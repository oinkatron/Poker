#ifndef UDPC_H
#define UDPC_H

#include <unistd.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <netdb.h> //Address Resolution
#include <sys/socket.h> //Socket call
#include <netinet/in.h> //sockaddr_in definition
#include <arpa/inet.h> //hton, ntoh etc

#include <errno.h>

#include "util.h"

#define VERSION_MINOR 3
#define VERSION_MAJOR 0

#define MAX_MSG_LEN 1024
#define MAX_CLI 8

#define PACKET_HEAD_SIZE 25

#define PKT_FLAG_CONNECT 0x01
#define PKT_FLAG_DISC 0x02
#define PKT_FLAG_PING 0x04
#define PKT_FLAG_RPLY 0x08
#define PKT_FLAG_QUERY 0x10

#define ACK_BUFF_LEN 32
#define DEFAULT_CONN_TIMEOUT 3000 //3 second timeout

struct ServInfo {
	struct sockaddr_in s_addr;
	char s_name[32];
	
	uint8_t pass_protect;

	uint8_t min_version;
	uint8_t maj_version;

	uint8_t n_players;
	uint8_t max_players;

};

struct Pckt {
	long sent_t;

	char flags;
	int seq_id;

	unsigned short size;
	char *payload;
};

struct RecvPckt {
	long recv_t;
	char flags;
	long cli_id;
	int seq_id;

	struct sockaddr_in origin;

	int ack_seq;
	int ack_bits;

	unsigned short size;
	char *payload;
};

void PktFree(struct RecvPckt* p);

struct CliData {
	long id;
	struct sockaddr_in addr;

	//timing data
	long last_rcv_time;

	int avg_rtt;
	int total_rtt;

	struct Pckt ack_buff[ACK_BUFF_LEN];
	int ack_cnt;

	unsigned int l_seq;

	unsigned int ack_seq;
	unsigned int ack_bits;

	unsigned int n_recvd;
	unsigned int n_sent;
	unsigned int n_acked;
};

struct Server { //Sends and Recieves Multiple Clients
   int sock;
	struct sockaddr_in b_addr;

	char name[32];
	char password_str[64];

	long start_time;
	long cur_time;
	
	long c_timeout;

	short max_cli;
	short num_cli;
	struct CliData *clients;

	unsigned short max_len;
	char *recv_buff;
};

struct Server *ServCreate(char *addr, short portno, short max_c, unsigned short buff_size);
void ServFree(struct Server *s);

long ServGenId();

int ServSend(struct Server *s, struct CliData *c, char f, char *buff, unsigned short buff_len);
int ServPcktSend(struct Server *s, struct CliData *c, struct Pckt *p);

struct RecvPckt* ServRecv(struct Server *s, int timeout);

struct CliData* ServAddCli(struct Server *s, struct sockaddr_in *addr);
struct CliData* ServGetCli(struct Server *s, long c_id);
int ServRemoveCli(struct Server *s, long c_id);

void ServSendInfo(struct Server *s, struct sockaddr_in *r);

void ServProcessPckt(struct Server *s, struct CliData *c, struct RecvPckt *p);

void ServTimeoutClients(struct Server *s);
void ServCheckDropped(struct Server *s);

/*
** CLIENT DATA STRUCTURES
*/

struct CliConn {
	int sock;
	struct sockaddr_in addr;

	long id;

	long cur_time;
	long start_time;
	long c_timeout;

	struct Pckt ack_buff[ACK_BUFF_LEN];
	int unacked[32];
	int ack_cnt;

	unsigned int l_seq;

	unsigned int ack_seq;
	unsigned int ack_bits;

	unsigned int n_recv;
	unsigned int n_sent;

	unsigned short max_len;
	char *recv_buff;
};

struct CliConn* CliConnect(char *addr, short portno, int timeout);

int CliSend(struct CliConn *c, char f, char *buff, unsigned short buff_len);
struct RecvPckt* CliRecv(struct CliConn *c, int timeout);

struct ServInfo* CliQueryServ(char *addr, int timeout);

void CliProcessAck(struct CliConn *c, struct RecvPckt *p);

/*
** PACKET STRUCTURES
*/

#endif
