#ifndef TBUI_H
#define TBUI_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <termbox.h>

#include "list.h"

typedef uint32_t* tb_str;

#define STR_ZERO(s,len) s = malloc(sizeof(int)*((len)+1)); memset(s,0,sizeof(int)*(len)+1)

#define MAX(a,b) ((a) > (b)) ? (a) : (b)
#define MIN(a,b) ((a) > (b)) ? (b) : (a)

void char_to_str(tb_str dest, const char *c, uint16_t max_len);
void str_to_char(char *dest, tb_str src, uint16_t max_len);

void tb_printf(const char *c, int s_x, int s_y, uint16_t fg, uint16_t bg);

int tb_strlen(tb_str s);
void tb_strcpy(tb_str dest, uint16_t cap, tb_str src, uint16_t len);
void tb_charcpy(tb_str dest, const char *src, uint16_t cap);
tb_str tb_str_insert(tb_str src, const char *appnd, uint16_t pos);
void tb_str_print(tb_str s, int s_x, int s_y, uint16_t fg, uint16_t bg);

#define UI_STATE_NONE 0
#define UI_STATE_HIGHL 1
#define UI_STATE_FOCUS 2

#define TB_WIDG 1
#define TB_WIDG_TEXTBOX 2
#define TB_WIDG_TEXTAREA 3
#define TB_WIDG_LABEL 4

void tb_clear_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height);

struct tb_widget {
	struct tb_widget* parent;
	int16_t index;

	uint16_t x;
	uint16_t y;

	uint16_t width;
   uint16_t  height;

	uint16_t fg;
	uint16_t  bg;

	uint8_t visible;
	uint8_t selectable;
	
	//callbacks
	void (*cb_action)(struct tb_widget*);

	void (*cb_focus)(struct tb_widget*); 
	void (*cb_unfocus)(struct tb_widget*); 

	void (*cb_select)(struct tb_widget*); 
	void (*cb_unselect)(struct tb_widget*); 

	//dynamic memory
	tb_str text;
	uint16_t cap;;

	void *widget;
	char type;
	char state;
};

void tb_widget_render(struct tb_widget *w);
void tb_widget_free(struct tb_widget *w);

void tb_widget_setText(struct tb_widget *w, const char *str);
int16_t tb_widget_getText(char *dest, struct tb_widget *w, uint16_t cap);

void tb_widget_handleInput(struct tb_widget *ui, struct tb_event *e);

void tb_widget_unfocus(struct tb_widget *w);
void tb_widget_focus(struct tb_widget *w);
void tb_widget_select(struct tb_widget *w);
void tb_widget_unselect(struct tb_widget *w);

struct tb_win {
	uint8_t draw_border;

	struct List *child_list;
	struct L_node *sel_child;
	struct tb_widget *f_child;
};

struct tb_widget* tb_win_create(const char *title, uint16_t ww, uint16_t hh);
void tb_win_free(struct tb_win *w);
void tb_win_render(struct tb_widget *w, int x, int y);

void tb_win_addChild(struct tb_widget *win, struct tb_widget *child);
void tb_win_removeChild(struct tb_widget *w, struct tb_widget *r);
void tb_win_set_position(struct tb_widget *w, uint16_t x, uint16_t y);

struct tb_textbox {
	uint16_t l_offset;
	int16_t c_pos;
};

struct tb_widget *tb_textbox_create(uint16_t width, uint16_t buff_len, uint16_t fg, uint16_t bg);
void tb_textbox_free(struct tb_textbox *tb);

void tb_textbox_render(struct tb_widget *tb, uint16_t xx, uint16_t yy);

void tb_textbox_set_cursorpos(struct tb_widget *tb, uint16_t pos);
void tb_textbox_move_cursor(struct tb_widget *tb, int16_t dx);
int16_t tb_textbox_get_cursorpos(struct tb_widget *tb);

void tb_textbox_set_text(struct tb_widget *tb, const char *c);
tb_str tb_textbox_get_text(struct tb_widget *tb);

void tb_textbox_insert_char(struct tb_widget *tb, uint32_t ch);
void tb_textbox_backspace(struct tb_widget *tb);
void tb_textbox_del_char(struct tb_widget *tb);

struct tb_label {
	uint16_t l_offset;
};

struct tb_widget *tb_label_create(const char *ptr, uint16_t w,  uint16_t fg, uint16_t bg);
void tb_label_free(struct tb_label *l);

void tb_label_render(struct tb_widget *l, int x, int y);

void tb_label_move_offset(struct tb_widget *w, uint16_t dx);
void tb_label_set_offset(struct tb_widget *w, uint16_t xx);
uint16_t tb_label_get_offset(struct tb_widget *w);

struct List ui_list;
struct L_node *sel_widg;
struct tb_widget *f_widg;

int tb_ui_init(); //setup termbox and ui initialisations
void tb_ui_shutdown(); //shutdown termbox and free ui data

void tb_ui_render();
void tb_ui_processInput(struct tb_event *e);

void tb_ui_setFocus(struct tb_widget *w);
void tb_ui_setSelection(struct tb_widget *w);

void tb_ui_addChild(struct tb_widget *w);
void tb_ui_removeChild(struct tb_widget *w);
void tb_ui_clear();


#endif
