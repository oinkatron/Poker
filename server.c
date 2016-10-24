//STD libraries
#include <stdio.h>
#include <stdlib.h>

//errno
#include <errno.h>

//Networking
#include "channel.h"
#include "util.h"

//Cards funcs
#include "poker.h"

#define FLUSH(b) memset((b), 0, sizeof(b));

#define SERVER_FPS 5
#define NET_FPS 10

#define POKER_STATE_PAUSED
#define POKER_STATE_HANDPREP //waiting for client seeds

struct PokerPlayer {
	uint8_t active; //if player a player has connected
	uint8_t in_game; //if player is in current hand

	uint64_t sess_id;
	uint64_t h_seed;

	int32_t chips; //equals chips available to bet
	int32_t bet; //if this number > 0 assume its been subtracted from chips

	Card hand[2];

	char nickname[32];
};

//Server structures
struct PokerGame {
	uint8_t state;
	uint64_t hand_seed;

	Card board[5];

	struct PokerPlayer *players;
	uint8_t n_players;
	uint8_t max_players;

	struct List *hand_players;
	
	uint8_t dealer_id;
	uint8_t sb_id;
	uint8_t bb_id;

	int8_t action_id;

	int32_t start_bb;
	int32_t cur_bb;

	uint64_t blind_millis;

	uint64_t last_action;
	uint64_t action_timeout;
};

struct PokerGame *PG_Create(uint8_t mx_players, int32_t starting_bb, uint64_t blind_millis);

int8_t PG_AddPlayer(uint64_t sess_id, int32_t starting_chips);
struct PokerPlayer *PG_GetPlayer(uint64_t sess_id);

//Game Actions
void PG_NewHand(struct PokerGame *pg) {
	if (!pg) {
		return;
	}

	//reset board
	for (int i=0; i<5; ++i) {
		pg->board[i] = 52;
	}

	int8_t next_dealer=-1;

	//Reset all players states
	for (int i=pg->max_players-1; i >= 0  --i) {
		pg->players[i].in_game = 0;
		//invalidate hand
		pg->players[i].hand[0] = 52;
		pg->players[i].hand[1] = 52;

		pg->players[i].h_seed = 0;
		pg->players[i].bet = 0;

		//Check if this is next dealer
		if (pg->players[i].active) {
			if (i > pg->dealer_id || (i < pg->dealer_id && (next_dealer < dealer_id && next_dealer > 0))) { //if no dealer has been set choose the oldest dealer
				next_dealer = i;
			}
		}
	}

	pg->dealer_id = next_dealer;
}


void PG_PostBlinds(struct PokerGame *pg);

//Server Variables
char serv_name[32];
char pass_str[64];

#define REQ_CONN 0
#define REQ_PASSWD 1
#define NOTI_CONNACCPT 2
#define NOTI_CONNREJCT 3
#define NOTI_PASS 4

void cliMsgRcvd(struct Channel *c, int16_t h_id, struct RecvPckt *p) {
	struct HostData *hd = &c->hosts.hosts[h_id];
	char nick[p->payload[1]+1];
	switch(p->payload[PCKT_HEAD_SIZE]) {
		case REQ_CONN:
			memcpy(nick, p->payload+2, p->payload[1]);
			nick[p->payload[1]] = 0;
			printf("%s wants to joind the game\n", nick);
			
			break;
	}
}

void query(struct Channel *c, struct RecvPckt *r) {
	if (c && r && r->payload[0] == 1) {
		uint8_t len = strlen(serv_name);
		
		char info_buff[len+7];

		info_buff[0] = 1;
		info_buff[1] = MAJOR_VERSION;
		info_buff[2] = MINOR_VERSION;
		info_buff[3] = n_players;
		info_buff[4] = max_players;
		
		if (strlen(pass_str) > 0) {
			info_buff[5] = 1;
		} else {
			info_buff[5] = 0;
		}

		info_buff[6] = len;
		strcpy(info_buff+7, serv_name);

		Channel_SendRaw(c, &r->origin, PCKT_FLAG_QUERYACK, info_buff, len+7);
	}
}

int main (int argn, char *args[]) {	
	//Seed RNGesus
	srand((unsigned)time(0));

	//Channel Init
	struct Channel *chan = Channel_Open("127.0.0.1:8008", 24, 256, 2500);
	if (!chan) {
		return 1;
	}

	chan->on_query = &query;
	chan->on_msg = &cliMsgRcvd;

	memset(pass_str, 0, 64);
	memset(serv_name, 0, 32);
	strcpy(serv_name, "Mooooo");

	//timing vars
	long last_t, start_t, end_t;
	long dt;

	while (1) {
		last_t = start_t;
		start_t = millitime();
		dt = start_t - last_t;

		Channel_Update(chan, start_t);
		while(Channel_Recv(chan, 0)) {}

		//Finally sleep for required time
		end_t = millitime();	
		
		if (end_t - start_t < 1000 / SERVER_FPS) {
			millisleep((1000 / SERVER_FPS) - (end_t - start_t));
		}
	}

	Channel_Close(chan);
  	return 0;
}
