#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <openssl/sha.h>

#include <sys/select.h>
#include <sys/time.h>

#include "channel.h"
#include "list.h"
#include "util.h"

#include "tb_ui.h"

#define MAX_CONNECTIONS 10

#define CLUB_SYMBOL 0x2663
#define SPADE_SYMBOL 0x2660
#define HEART_SYMBOL 0x2665
#define DIAMOND_SYMBOL 0x2666

#define ERROR_LINE tb_height()-3
#define INFO_LINE tb_height()-2

#define TB_INFO(c) tb_clear_rect(1, INFO_LINE, tb_width()-1, 1);\
		  				  tb_printf((c), 1, INFO_LINE, 0, 0);
#define TB_ERROR(c) tb_clear_rect(1, ERROR_LINE, tb_width()-1, 1);\
		  					tb_printf((c), 1, ERROR_LINE, TB_REVERSE, 0)

#define CLI_FPS 10

#define REQ_CONN 0
#define REQ_PASSWD 1
#define NOTI_CONNACCPT 2
#define NOTI_CONNREJCT 3
#define NOTI_PASS 4

#define GAME_JOINREQ 1
#define GAME_JOINED 2
#define GAME_PLAYING 3
#define GAME_SPECTATIING 4

//////////////////
//Client Structures
///////////////////

struct GameInfo {
	uint64_t c_id;
	uint8_t state;
	struct ServInfo *serv;
};

struct ServInfo {
	char name[32];
	struct sockaddr_in addr;

	uint8_t min_v, maj_v;
	uint8_t n_players, max_players;
	uint8_t pass_protected;
};

struct tb_widget *serv_info_win(struct ServInfo *si);

///////////////////////
//VARIABLES & Constants
///////////////////////

#define MAX_GAMES 10
#define MAX_CONNS 12

#define CLI_STATE_DEF 1
#define CLI_STATE_SELSERV 2
#define CLI_STATE_ADDSERV 3

struct GameInfo games[MAX_GAMES];
int n_games;

struct Channel *s_chan;

int8_t view_state;
int8_t cli_state;

//UI VARS
struct tb_widget *add_serv_tb;
struct tb_widget *lbl_info, *lbl_error;

struct tb_widget *win_servlist;

struct tb_widget *serv_info_list[MAX_CONNS];
int serv_info_num = 0;

void setState(int8_t s) {
	switch(cli_state) { //proper state deinitialization
		case CLI_STATE_DEF:
			break;
		case CLI_STATE_ADDSERV:
			tb_ui_removeChild(add_serv_tb);
			break;
		case CLI_STATE_SELSERV:
			break;
	}

	switch(s) {
		case CLI_STATE_DEF:
			tb_widget_setText(lbl_info, "Welcome to DRC Client Alpha 0.1");
			break;
		case CLI_STATE_ADDSERV:
			tb_widget_setText(lbl_info, "Enter IP:Port to query a server");
			tb_ui_addChild(add_serv_tb);
			tb_ui_setFocus(add_serv_tb);
			add_serv_tb->x = 34;
			break;
		case CLI_STATE_SELSERV:
			tb_widget_setText(lbl_info, "[Enter]: Join	[S]pectate	[R]emove");
			break;
	}

	cli_state = s;
}

//callback functions

void connEstablished(struct Channel *ch, int16_t h_id, int8_t connected) {
	if (connected) {
		//Static Nickname: NickNoley
		char buff[11];
		buff[0] = REQ_CONN;
		buff[1] = 9;
		memcpy(buff+2, "NickNoley", 9);
		int n = Channel_Send(ch, &ch->hosts.hosts[h_id], PCKT_FLAG_MSG, buff, 11);
		printf("Sent %d bytes\n", n);
	} else {
		tb_widget_setText(lbl_error, "Unable to establish connection!");
	}
}

void infoWinHighlight(struct tb_widget* w) {
	setState(CLI_STATE_SELSERV);
}

void infoWinFocus(struct tb_widget *w) {
	//Establish full connection to server and
	//make a game with info
	char addr_str[18];
	tb_widget_getText(addr_str, List_Get(((struct tb_win*)w->widget)->child_list, 2), 18);
	Channel_Connect(s_chan, addr_str, &connEstablished);
	
}

void addServAction(struct tb_widget *w) {
	if (s_chan) {
		char s_addr[24];
		if (tb_widget_getText(s_addr, w, 24) > 0) {
			Channel_QueryHost(s_chan, s_addr, 1);
		}
	}
}

void msgRecieved(struct Channel *ch, int16_t h_id, struct RecvPckt *rp) {
	printf("Recieved msg\n");
}

void queryResponse(struct Channel *c, struct RecvPckt *p) {
	if (p->payload[0] == 1 && serv_info_num < 10) {
		struct ServInfo *si = NEW(struct ServInfo);
		ZERO(si, struct ServInfo);

		memcpy(&si->addr, &p->origin, sizeof(struct sockaddr_in));

		si->maj_v = p->payload[1];
		si->min_v = p->payload[2];

		si->n_players = p->payload[3];
		si->max_players = p->payload[4];
		si->pass_protected = p->payload[5];
	
		memcpy(si->name, p->payload+7, (p->payload[6] <= 32) ? p->payload[6] : 32);

		serv_info_list[serv_info_num] = serv_info_win(si);
		serv_info_list[serv_info_num]->y = 3+((serv_info_num)*6);
		serv_info_list[serv_info_num]->x = 1;

		tb_win_addChild(win_servlist, serv_info_list[serv_info_num]);
		tb_ui_setSelection(serv_info_list[serv_info_num++]);
	}
}

//////////////
//STRUCT FUNCS
//////////////

struct tb_widget *serv_info_win(struct ServInfo *si) {
	if (!si) {
		return NULL;
	}
	
	char p_txt[16], l_txt[12], v_txt[32], a_txt[18];
	memset(p_txt, 0, 24);
	memset(l_txt, 0, 12);
	memset(v_txt, 0, 16);
	memset(a_txt, 0, 24);

	addrToStr(a_txt, &si->addr, 18);

	snprintf(v_txt, 32, "%s: [%d.%d]", si->name, si->min_v, si->maj_v);
	struct tb_widget *new_win = tb_win_create(v_txt, 20, 3);

	snprintf(p_txt, 16,  "%d / %d Players", si->n_players, si->max_players);
	snprintf(l_txt, 12, "Locked[y/n]: %c", (si->pass_protected) ? 'y' : 'n');
	
	struct tb_widget *a_lbl = tb_label_create(a_txt, 20, TB_BLUE, 0);
	struct tb_widget *p_lbl = tb_label_create(p_txt, 20, TB_GREEN, 0);
	struct tb_widget *l_lbl = tb_label_create(l_txt, 20, 0, 0);	

	p_lbl->x = 0;
	p_lbl->y = 0;
	l_lbl->x = 0;
	l_lbl->y = 1;
	a_lbl->x = 0;
	a_lbl->y = 2;

	tb_win_addChild(new_win, p_lbl);
	tb_win_addChild(new_win, l_lbl);
	tb_win_addChild(new_win, a_lbl);

	new_win->selectable = 1;
	new_win->cb_select = &infoWinHighlight;
	new_win->cb_focus = &infoWinFocus;

	((struct tb_win*)new_win->widget)->draw_border = 1;

	return new_win;
}


int main (int argn, char *args[]) {
	//redirect stdout
	fclose(stdout);
	stdout = fopen("log.txt", "w");

	fclose(stderr);
	stderr = fopen("err_log.txt", "w");

	//Channel init
	s_chan = Channel_Open("127.0.0.1:8009", 1, 256, 2500);
	if (!s_chan) {
		return -1;
	}

	s_chan->on_query_response = &queryResponse;
	s_chan->on_msg = &msgRecieved;

	//tb_ui init
	if (tb_ui_init() < 0) {
		printf("Error initialising tb_ui!");
		return -1;
	}

	//UI Definition
	win_servlist = tb_win_create("Server List", 24, tb_height()-6);
	win_servlist->x = 1;
	win_servlist->y = 3;

	win_servlist->selectable = 1;
	((struct tb_win*)win_servlist->widget)->draw_border = 1;

	add_serv_tb = tb_textbox_create(20, 128, TB_GREEN, 0);

	add_serv_tb->x = 24;
	add_serv_tb->y = tb_height()-1;
	add_serv_tb->selectable = 1;

	add_serv_tb->cb_action = &addServAction;

	lbl_info = tb_label_create("", tb_width(), 0, 0);
	lbl_error = tb_label_create("", tb_width(), TB_REVERSE, 0);

	lbl_info->x = 0;
	lbl_info->y = tb_height()-1;
	lbl_error->x = 0;
	lbl_error->y = tb_height()-2;

	tb_ui_addChild(lbl_info);
	tb_ui_addChild(lbl_error);
	tb_ui_addChild(win_servlist);

	setState(CLI_STATE_DEF);
	tb_ui_render();
	tb_present();

	//timing vars
	uint64_t last_t = 0,
				start_t = millitime(),
				dt = start_t - last_t,
				mid_t = 0,
				end_t = 0;

	struct tb_event e;

	uint8_t tb_had_event=0;
	uint64_t c_start, c_end;
	uint64_t tb_start, tb_end;

	while (1) { //event loop
		tb_had_event=0;
		
		last_t = start_t;
		start_t = millitime();
		dt = start_t - last_t;


		//Check for netstuffs
		Channel_Update(s_chan, start_t);
		while (Channel_Recv(s_chan, 0)) {
			tb_had_event=1;
		}

		mid_t = millitime();

		//Check for local termbox events
		while (tb_peek_event(&e, 0)) {
			switch (e.type) {
				case TB_EVENT_KEY:
					tb_had_event=1;
					if (e.ch != 0) { //Regular keypress
						switch (e.ch) {
						case ((int)'a'):
							if (cli_state == CLI_STATE_DEF) {
								setState(CLI_STATE_ADDSERV);
							}
							break;
						default:
							tb_ui_processInput(&e);
							break;
						}	  	  
					} else {
						switch (e.key) {
						case TB_KEY_CTRL_C:
							tb_ui_shutdown();
							Channel_Quit(s_chan, 0);
						default:
							tb_ui_processInput(&e);
							break;
						}	  
					}
					break;
			}
		}
			
		if (tb_had_event) 
			tb_ui_render(); //refresh screen

		end_t = millitime();
		if (end_t - start_t < 1000 / CLI_FPS) {
			millisleep((1000 / CLI_FPS) - (end_t - start_t));
		}
	}

	tb_ui_shutdown();

   return 0;
}
