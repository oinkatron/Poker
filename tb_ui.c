#include "tb_ui.h"

#define MAX(a,b) ((a) > (b)) ? (a) : (b)
#define MIN(a,b) ((a) > (b)) ? (b) : (a)

int tb_ui_init() {
	int ret = tb_init();
	if (ret < 0) {
		printf("error initialising termbox, error code: %d\n", ret);
		return ret;
	}
	
	tb_clear();

	//setup ui_list
	memset(&ui_list, 0, sizeof(struct List));
	return 0;
}

void tb_ui_shutdown() {
	tb_shutdown();

	struct L_node *n = ui_list.head;
	while (n) {
		tb_widget_free(n->data);
		n = n->next;
	}

	return;
}

void tb_ui_processInput(struct tb_event *e) {
	struct tb_widget *new_widg = NULL;	 
	struct L_node *new_n = NULL;

	switch (e->type) {
		case TB_EVENT_KEY:
			if (f_widg) {
				tb_widget_handleInput(f_widg, e);
			} else {
				switch (e->key) {
					case TB_KEY_ARROW_RIGHT:
						if(sel_widg) {
							new_n = sel_widg->next;
							while (new_n) {
								if(!new_n || ((struct tb_widget*)new_n->data)->selectable) {
									break;
								}
								new_n = new_n->next;
							}

							tb_widget_unselect(sel_widg->data);	
							sel_widg = new_n;
						} else {
							sel_widg = ui_list.head;
							while(sel_widg) {
								if(!sel_widg || ((struct tb_widget*)sel_widg->data)->selectable) {
									break;
								}
								
								sel_widg = sel_widg->next;
							}
							
						}

						if (sel_widg) {
							tb_widget_select(sel_widg->data);
						}
						break;
					case TB_KEY_ARROW_LEFT:
						if(sel_widg) {
							new_n = sel_widg->prev;
							while (new_n) {
								if(!new_n || ((struct tb_widget*)new_n->data)->selectable) {
									break;
								}
								new_n = new_n->prev;
							}

							tb_widget_unselect(sel_widg->data);	
							sel_widg = new_n;
						} else {
							sel_widg = ui_list.tail;
							while(sel_widg) {
								if(!sel_widg || ((struct tb_widget*)sel_widg->data)->selectable) {
									break;
								}
								sel_widg = sel_widg->prev;
							}
							
						}

						if (sel_widg) {
							tb_widget_select(sel_widg->data);
						}
						break;	
					case TB_KEY_ENTER:
						if (sel_widg) {
							f_widg = sel_widg->data;
							tb_widget_focus(f_widg);
							sel_widg = NULL;
						}
						break;
				}
			}
		break;
	}
}

void tb_ui_render() {
	if (ui_list.size == 0)
		return;

	struct L_node *ln;
	struct tb_widget *w;

	ln = ui_list.head;
	if (!ln) {
		return;
	}
	
	tb_clear();

	while(ln) {
		w = ln->data;
		if (!w) {
			ln = ln->next;
			continue;
		}
		tb_widget_render(w);
		ln = ln->next;
	}

	tb_present();
}

void tb_ui_setFocus(struct tb_widget *w) {
	if (f_widg) {
		tb_widget_unfocus(f_widg);
	}

	f_widg = w;
	tb_widget_focus(f_widg);
}


void tb_ui_setSelection(struct tb_widget *w) {
	if (sel_widg) {
		tb_widget_unselect(sel_widg->data);
	}

	if (w) {
		if (w->parent) {
			if (f_widg != w->parent) {
				tb_ui_setFocus(w->parent);
			}
			sel_widg = List_GetNode(((struct tb_win*)w->parent->widget)->child_list, w->index);
		} else {
			tb_ui_setFocus(NULL);
			sel_widg = List_GetNode(&ui_list, w->index);
		}

		tb_widget_select(sel_widg->data);
	} else {
		sel_widg = NULL;
	}

}

void tb_ui_addChild(struct tb_widget *w) {
	w->index = ui_list.size;
	List_Push(&ui_list, w);
}

void tb_ui_removeChild(struct tb_widget *w) {
	if (ui_list.size == 0)
		return;

	int cnt=0;
	for (struct L_node *ln = ui_list.head; ln != NULL; ln = ln->next) {
		if (ln->data == w) {
			w->index = -1;
			List_Remove(&ui_list, cnt);
			return;
		}
		cnt++;
	}
}

void tb_ui_clear() {
	if (ui_list.size == 0) 
		return;

	struct L_node *ln;
	int cnt = ui_list.size-1;
	for (ln = ui_list.tail; ln != NULL; ln = ln->prev) {
		tb_widget_free(ln->data);
		List_Remove(&ui_list, cnt);
		--cnt;
	}
}

void tb_clear_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
	struct tb_cell blank;
	struct tb_cell *buff = tb_cell_buffer();


	blank.fg=0;
	blank.bg=0;
	blank.ch=0;
	
	for (int ii=x; ii < x+width; ii++) {
		for (int i=y; i < y+height; i++) {
			tb_put_cell(ii, i, &blank);	
		}
	}

	tb_present();
}

void char_to_str(tb_str dest, const char *c, uint16_t max_len) {
	
	int max_cpy = MIN(max_len-1, strlen(c));
	
	for (int i=0; i < max_cpy; i++) {
		tb_utf8_char_to_unicode(&dest[i], (c+i));
	}

	dest[max_cpy] = 0;
}

void str_to_char(char *dest, tb_str src, uint16_t max_len) {
	int max_cpy = MIN(max_len-1, tb_strlen(src));

	for (int i=0; i < max_cpy; i++) {
		tb_utf8_unicode_to_char(&dest[i], *(src+i));
	}

	dest[max_cpy] = 0;
}

int tb_strlen(tb_str s) {
	int c=0;
	while (*(s++))
		++c;

	return c;
}

void tb_strcpy(tb_str dest, uint16_t cap, tb_str src, uint16_t len) {
	//find the copy length
	uint16_t n_cpy = (cap > len) ? cap : len;

	memcpy(dest, src, n_cpy*sizeof(int));
}

void tb_charcpy(tb_str dest, const char *src, uint16_t cap) {
	int16_t len = MIN(strlen(src), cap);
	for (int i=0; i < len; ++i) {
		if (src[i] == '\n' || src[i] == '\t' || src[i] == '\r') {
			tb_utf8_char_to_unicode(&dest[i], " ");
		} else {
			tb_utf8_char_to_unicode(&dest[i], &src[i]);
		}
	}
}

tb_str tb_str_insert(tb_str src, const char *appnd, uint16_t pos) {
	if (!src || !appnd) {
		return NULL;
	}

	tb_str new_s;
	uint16_t n_cnv = strlen(appnd);
	uint16_t n_pre = tb_strlen(src);
	
	STR_ZERO(new_s, n_pre+n_cnv);

	if (pos >= tb_strlen(src)) {
		tb_strcpy(new_s, n_pre+n_cnv, src, n_pre);
		tb_charcpy(new_s+n_pre, appnd, n_cnv);
	} else {
		tb_strcpy(new_s, n_pre+n_cnv, src, pos);
		tb_charcpy(new_s+pos, appnd, n_pre+n_cnv-pos);
		tb_strcpy(new_s+pos+n_cnv, n_cnv-pos, src+pos, n_pre-pos);
	}

	return new_s;
}

void tb_printf(const char *c, int s_x, int s_y, uint16_t fg, uint16_t bg) {
	int xx = s_x;
	int yy = s_y;

	uint32_t ch;

	while (*c) {
		tb_utf8_char_to_unicode(&ch, c);
		tb_change_cell(xx, yy, ch, fg, bg);

		if (++xx >= tb_width()) {
			xx = s_x;
			if (++yy >= tb_height()) {
				//no more space quit early
				return;
			}
		}
		++c;
	}	
}

void tb_str_print(tb_str s, int s_x, int s_y, uint16_t fg, uint16_t bg) {
	int xx = s_x;
	int yy = s_y;

	while (*s++) {
		tb_change_cell(xx, yy, *(s-1), fg, bg);

		if (++xx >= tb_width()) {
			xx = s_x;
			if (++yy >= tb_height()) {
				//no more space quit early
				return;
			}
		}
	}
}

void tb_widget_setText(struct tb_widget *w, const char *str) {
	if (!w || !str) {
		return;
	}

	int s_len = strlen(str);
	if (s_len >= w->cap) {
		free(w->text);
		STR_ZERO(w->text, s_len);
		w->cap = s_len+1;
	}

	tb_charcpy(w->text, str, w->cap);
}

int16_t tb_widget_getText(char *dest, struct tb_widget *w, uint16_t cap) {
	if (!w || !dest) {
		return -1;
	}

	int s_len = MIN(tb_strlen(w->text), cap);
	int cnt;

	for (int i=0; i<s_len; ++i) {
		tb_utf8_unicode_to_char((dest+i), w->text[i]);
		++cnt;
	}

	dest[s_len]=0;

	return cnt;
}

void tb_widget_free(struct tb_widget *w) {
	if (!w) {
		return;
	}

	if (w->text) {
		free(w->text);
	}

	switch(w->type) {
		case TB_WIDG:
			tb_win_free(w->widget);
			break;
		case TB_WIDG_LABEL:
			tb_label_free(w->widget);
			break;
		case TB_WIDG_TEXTBOX:
			tb_textbox_free(w->widget);
			break;
	}
}

void tb_widget_render(struct tb_widget *w) {
	if (!w) {
		return;
	}
	
	switch(w->type) {
		case TB_WIDG:
			tb_win_render(w, w->x, w->y);
			break;
		case TB_WIDG_LABEL:
			tb_label_render(w, w->x, w->y);
			break;
		case TB_WIDG_TEXTBOX:
			tb_textbox_render(w, w->x, w->y);
			break;
	}
}

void tb_widget_handleInput(struct tb_widget *ui, struct tb_event *e) {
	if (!e || !ui) {
		return;
	}

	struct tb_win *win = NULL;
	struct L_node *new_n = NULL;

	switch(ui->type) {
		case TB_WIDG:
			win = ui->widget;
			switch(e->key) {
				case TB_KEY_ESC:
					tb_widget_unfocus(ui);
					f_widg = ui->parent;
					tb_widget_focus(f_widg);

					if (sel_widg) {
						tb_widget_unselect(sel_widg->data);
						sel_widg = NULL;
					}
					break;
				case TB_KEY_ARROW_LEFT:
					win = ui->widget;
					if(sel_widg) {
						new_n = sel_widg->prev;
						while(1) {
							if(!new_n || ((struct tb_widget*)new_n->data)->selectable) {
								break;
							}
							new_n = new_n->prev;
						}

						tb_widget_unselect(sel_widg->data);
						sel_widg = new_n;
					} else {
						for(sel_widg = win->child_list->tail;
								sel_widg != NULL;
									sel_widg = sel_widg->prev) {
							if (((struct tb_widget*)sel_widg->data)->selectable) {
								break;
							}
						}
					}

					if (sel_widg) {
						tb_widget_select(sel_widg->data);
					}
					break;
				case TB_KEY_ARROW_RIGHT:
					win = ui->widget;
					if(sel_widg) {
						for(new_n = sel_widg->next;
							new_n != NULL;
							new_n = new_n->next) {
							if (((struct tb_widget*)new_n->data)->selectable) {
								break;
							}
						}

						tb_widget_unselect(sel_widg->data);
						sel_widg = new_n;
					} else {
						for(sel_widg = win->child_list->head;
								sel_widg != NULL;
									sel_widg = sel_widg->next) {
							if (((struct tb_widget*)sel_widg->data)->selectable) {
								break;
							}
						}
					}

					if (sel_widg) {
						tb_widget_select(sel_widg->data);
					}
					break;
				case TB_KEY_ENTER:
					if (sel_widg) {
						f_widg = sel_widg->data;
						
						tb_widget_focus(f_widg);
						tb_widget_unselect(sel_widg->data);
						
						sel_widg = NULL;
					}
					break;
			}
			break;
		case TB_WIDG_LABEL:
			switch (e->key) {	
				case TB_KEY_ESC:
					tb_widget_unfocus(ui);	
					f_widg = ui->parent;
					break;
				case TB_KEY_ARROW_LEFT:
					tb_label_move_offset(ui, -1);
					break;
				case TB_KEY_ARROW_RIGHT:
					tb_label_move_offset(ui, 1);
					break;
			}
			break;
		case TB_WIDG_TEXTBOX:
			switch(e->key) {
				case TB_KEY_ESC:
					tb_widget_unfocus(ui);	
					f_widg = ui->parent;
					break;
				case TB_KEY_ARROW_LEFT:
					tb_textbox_move_cursor(ui, -1);	
					break;
				case TB_KEY_ARROW_RIGHT:
					tb_textbox_move_cursor(ui, 1);	
					break;
				case TB_KEY_DELETE:
					tb_textbox_del_char(ui);
					break;
				case TB_KEY_BACKSPACE2:
					tb_textbox_backspace(ui);
					break;
				case TB_KEY_SPACE:
					tb_textbox_insert_char(ui, (int)' ');
					break;
				case TB_KEY_ENTER:
					if (ui->cb_action) {
						(*ui->cb_action)(ui);
					}
					break;
				default:
					if (e->ch) {
						tb_textbox_insert_char(ui, e->ch);
					}
					break;
			}
			break;
	}
}

void tb_widget_unfocus(struct tb_widget *w) {
	if (!w) {
		return;
	}

	if (w->cb_unfocus) {
		(*w->cb_unfocus)(w);
	}

	w->state = UI_STATE_NONE;
}

void tb_widget_focus(struct tb_widget *w) {
	if (!w) {
		return;
	}

	if (w->cb_focus) {
		(*w->cb_focus)(w);
	}

	w->state = UI_STATE_FOCUS;
}

void tb_widget_select(struct tb_widget *w) {
	if (!w) {
		return;
	}

	if (w->cb_select) {
		(*w->cb_select)(w);
	}

	w->state = UI_STATE_HIGHL;
}

void tb_widget_unselect(struct tb_widget *w) {
	if (!w) {
		return;
	}

	if (w->cb_unselect) {
		(*w->cb_unselect)(w);
	}

	w->state = UI_STATE_NONE;
}

struct tb_widget *tb_win_create(const char *title, uint16_t ww, uint16_t hh) {
	struct tb_widget *n_w = malloc(sizeof(struct tb_widget));
	if (!n_w) {
		return NULL;
	}	
	memset(n_w, 0, sizeof(struct tb_widget));

	n_w->type = TB_WIDG;
	n_w->index = -1;

	if (ww >= tb_width()) {
		n_w->width = tb_width()-1;
	} else {
		n_w->width = ww;
	}

	if (hh >= tb_height()) {
		n_w->height = tb_height()-1;
	} else {
		n_w->height = hh;
	}

	struct tb_win *wi = malloc(sizeof(struct tb_win));
	memset (wi, 0, sizeof(struct tb_win));
	
	STR_ZERO(n_w->text, strlen(title));
	n_w->cap = strlen(title);
	char_to_str(n_w->text, title, strlen(title)+1);

	wi->child_list = malloc(sizeof(struct List));

	n_w->widget = wi;
	return n_w;
}

void tb_win_free(struct tb_win *w) {
	if (!w) {
		return;
	}

	if (w->child_list) {
		struct L_node *n = w->child_list->head;
		while (n) {
			tb_widget_free(n->data);
			n = n->next;
		}

		List_Free(w->child_list);
	}

	free(w);
}

void tb_win_render(struct tb_widget *w, int x, int y) {
	if (!w || !w->widget || w->type != TB_WIDG) {
		return;
	}

	int xx = x;
	int yy = y;

	struct tb_cell t;
	t.ch = (int)'-';
	t.fg = TB_WHITE;
	
	switch(w->state) {
		case UI_STATE_NONE:
			t.bg = 0;
			break;
		case UI_STATE_HIGHL:
			t.bg = TB_BLUE;
			break;
		case UI_STATE_FOCUS:
			t.bg = TB_RED;
			break;
	}

	struct tb_win *win = w->widget;
	
	if (win->draw_border) {
		for (int i=xx; i < xx + w->width; i++) {
			tb_put_cell(i, yy-1, &t);
			tb_put_cell(i, yy-3, &t);
			tb_put_cell(i, yy+w->height, &t);
		}

		tb_str_print(w->text, xx+1, yy-2, w->fg, w->bg);

		t.ch = (int)'|';

		for (int ii=yy-2; ii <= yy + w->height; ii++) {
			tb_put_cell(xx-1, ii, &t);
			tb_put_cell(xx+w->width, ii, &t);
		}
	
		t.ch = (int)'+';

		tb_put_cell(xx-1, yy-1, &t); 
		tb_put_cell(xx+w->width, yy-1, &t); 
		tb_put_cell(xx-1, yy+w->height, &t); 
		tb_put_cell(xx+w->width, yy+w->height, &t); 
   }
	
	//Now fill inside
	t.ch = 0;
	if (!win->draw_border && w->state == UI_STATE_HIGHL) {
		t.fg = TB_BLUE;
	} else {
		t.fg = w->fg;
	}
	t.bg = w->bg;

	for (int i=xx; i< xx+w->width; i++) {
		for (int ii=yy; ii < yy+w->height; ii++) {
			tb_put_cell(i, ii, &t);
		}
	}

	//Now Render children
	struct tb_widget *cw;
	for (struct L_node *n = ((struct tb_win*)(w->widget))->child_list->head; n != NULL; n = n->next) {
		cw = n->data;
		if (cw) {
			cw->x += xx;
			cw->y += yy;
			
			tb_widget_render(cw);
			
			cw->x -= xx;
			cw->y -= yy;
		}
	}

	return;

}

void tb_win_addChild(struct tb_widget *win, struct tb_widget *child) {
	if (!win || win->type != TB_WIDG || !child) {
		return;
	}

	struct tb_win* w = win->widget;
	if (w) {
		child->parent = win;
		child->index = w->child_list->size;
		List_Push(w->child_list, child);
	}

}





void tb_win_removeChild(struct tb_widget *w, struct tb_widget *r) {
	if (!w || !r) {
		return;
	}

	struct tb_win* win = w->widget;
	if (!win) {
		return;
	}

	int cnt = 0;
	for (struct L_node *i = win->child_list->head; i != NULL; i = i->next) {
		if (i->data == r) {
			r->index = -1;
			List_Remove(win->child_list, cnt);
		}
		++cnt;
	}
}

struct tb_widget *tb_label_create(const char *ptr, uint16_t ww,  uint16_t fg, uint16_t bg) {
	struct tb_widget *n_l = malloc(sizeof(struct tb_widget));
	if (!n_l) {
		return NULL;
	}
	memset(n_l, 0, sizeof(struct tb_widget));

	struct tb_label *w  = malloc(sizeof(struct tb_label));

	if (!w) {
		free(n_l);
		return NULL;
	}
	memset(w, 0, sizeof(struct tb_label));

	n_l->type = TB_WIDG_LABEL;
	n_l->index = -1;

	n_l->width = strlen(ptr);
	if (ww > 0) {
		n_l->width = ww;
	}

	n_l->height = 1;

	n_l->fg = fg;
	n_l->bg = bg;

	n_l->cap = strlen(ptr)+1;
	STR_ZERO(n_l->text, n_l->cap);

	if (n_l->width < n_l->cap-2) {
		n_l->selectable = 1;
	}

	char_to_str(n_l->text, ptr, n_l->cap+1);
	n_l->widget = w;

	return n_l;
}


void tb_label_free(struct tb_label *l) {
	if (!l) {
		return;
	}

	free(l);
}


void tb_label_move_offset(struct tb_widget *w, uint16_t dx) {
	if (!w || w->type != TB_WIDG_LABEL) {
		return;
	}

	struct tb_label *lb = w->widget;
	if (lb) {
		lb->l_offset += dx;
		uint16_t str_l = tb_strlen(w->text);
	 	
		if ((int16_t)lb->l_offset < 0) {
			lb->l_offset = 0;
		} else if (lb->l_offset > str_l - w->width) {
			lb->l_offset = str_l - w->width;
		}
	}
}

void tb_label_set_offset(struct tb_widget *w, uint16_t xx) {
	if (!w || w->type != TB_WIDG_LABEL) {
		return;
	}

	struct tb_label *lb = w->widget;
	if (lb) {
		lb->l_offset = xx;
		uint16_t str_l = tb_strlen(w->text);
	 	
		if (lb->l_offset > str_l - w->width-1) {
			lb->l_offset = str_l - w->width-1;
		}
	}

}

uint16_t tb_label_get_offset(struct tb_widget *w) {
	if (!w || w->type != TB_WIDG_LABEL) {
		return 0;
	}

	struct tb_label *lb = w->widget;
	if (lb) {
		return lb->l_offset;	
	}

	return 0;

}

void tb_label_render(struct tb_widget *l, int x, int y) {
	if (!l || l->type != TB_WIDG_LABEL) {
		return;
	}
	
	struct tb_label* lb = l->widget;
	struct tb_cell cell;
	
	if (lb) {
		switch(l->state) {
			case UI_STATE_NONE:	 
				cell.bg = l->bg;
				cell.fg = l->fg;
				break;
			case UI_STATE_HIGHL:
				cell.bg = TB_BLUE;
				cell.fg = TB_WHITE;
				break;
			case UI_STATE_FOCUS:
				cell.bg = l->bg;
				cell.fg = l->fg;
				break;
		}
		int str_l = tb_strlen(l->text);

		for (int i=0; i<l->width-1; i++) {
			if (i+lb->l_offset < str_l) {
				cell.ch = l->text[i+lb->l_offset];
				tb_put_cell(x+i, y, &cell);
			} else {
				cell.ch = 0x0020; //SPACE
				tb_put_cell(x+i, y, &cell);
			}
		}
	
		if (l->width+lb->l_offset < str_l) { //More string after
				cell.ch = 0x002B; //+ SIGN
				tb_put_cell(x+l->width-1, y, &cell);
		} else if (l->width+lb->l_offset == str_l) { //Last char
				cell.ch = l->text[str_l-1];
				tb_put_cell(x+l->width-1, y, &cell);
		} else {
				cell.ch = 0x0020;
				tb_put_cell(x+l->width-1, y, &cell);
		}
	}
}

struct tb_widget *tb_textbox_create(uint16_t width, uint16_t buff_len, uint16_t fg, uint16_t bg) {
	struct tb_widget *n_w = malloc(sizeof(struct tb_widget));
	if (!n_w) {
		return NULL;
	}

	struct tb_textbox *n_tb = malloc(sizeof(struct tb_textbox));
	if (!n_tb) {
		free(n_w);
		return NULL;
	}

	n_w->type = TB_WIDG_TEXTBOX;
	n_w->width = width;
	n_w->height = 1;

	n_w->fg = fg;
	n_w->bg = bg;

	STR_ZERO(n_w->text, buff_len);
	n_w->cap = buff_len;

	n_tb->c_pos = 0;
	n_tb->l_offset = 0;

	n_w->widget = n_tb;
	return n_w; 
}

void tb_textbox_free(struct tb_textbox *tb) {
	if (!tb) {
		return;
	}

	free(tb);
}

void tb_textbox_render(struct tb_widget *tb, uint16_t xx, uint16_t yy) {
	struct tb_textbox *t = tb->widget;
 
	tb_str b_str = tb->text;
	short t_len = tb_strlen(b_str);

	struct tb_cell c;

	switch(tb->state) {
		case UI_STATE_NONE:
			c.bg = tb->bg;
			c.fg = tb->fg;
			break;
		case UI_STATE_HIGHL:
			c.bg = TB_BLUE;
			c.fg = TB_WHITE;
			break;
		case UI_STATE_FOCUS:
			c.bg = TB_WHITE;
			c.fg = tb->fg;
	}

	for (int i=0; i < tb->width; i++) {	
		if(i+t->l_offset == t->c_pos && tb->state == UI_STATE_FOCUS) {
			c.fg |= TB_REVERSE;
		} else {
			c.fg &= (!TB_REVERSE);
		}
		
		if (i+t->l_offset < t_len) {
			c.ch = b_str[i+t->l_offset];
		} else {
			c.ch = 0;
		}
		
		tb_put_cell(xx+i, yy, &c);
	}
}

void tb_textbox_set_cursorpos(struct tb_widget *tb, uint16_t pos) {
	if (!tb || tb->type != TB_WIDG_TEXTBOX) {
		return;
	}

	struct tb_textbox *b = tb->widget;
	if (b) {
		b->c_pos = pos;

		//confine range
		uint32_t max = tb_strlen(tb->text);
		if (b->c_pos > max) {
			b->c_pos = max;
		} else if (b->c_pos < 0) {
			b->c_pos = 0;
		}
		
		//shift text offset
		if (b->c_pos-1 < b->l_offset) {
			b->l_offset = MAX(b->c_pos-1, 0);
		} else if (b->c_pos >= (b->l_offset+tb->width)) {
			b->l_offset = b->c_pos-tb->width+1;
		}
	}

}

int16_t tb_textbox_get_cursorpos(struct tb_widget *tb) {
	if (!tb || tb->type != TB_WIDG_TEXTBOX) {
		return -1;
	}

	return ((struct tb_textbox*)(tb->widget))->c_pos;
}


void tb_textbox_move_cursor(struct tb_widget *tb, int16_t dx) {
	if (!tb || tb->type != TB_WIDG_TEXTBOX) {
		return;
	}

	struct tb_textbox *b = tb->widget;
	if (b) {
		b->c_pos += dx;

		//confine range
		int32_t max = (tb->cap-1 > tb_strlen(tb->text)) ? tb_strlen(tb->text) : tb->cap-1;
		if (b->c_pos > max) {
			b->c_pos = max;
		} else if (b->c_pos < 0) {
			b->c_pos = 0;
		}
		
		//shift text offset
		if (b->c_pos-1 < b->l_offset) {
			b->l_offset = MAX(b->c_pos-1, 0);
		} else if (b->c_pos >= (b->l_offset+tb->width)) {
			b->l_offset = b->c_pos-tb->width+1;
		}
	}
}

void tb_textbox_insert_char(struct tb_widget *tb, uint32_t ch) {
	if (!tb) {
		return;
	}

	struct tb_textbox *b = tb->widget;
	
	if (b) {
		if (b->c_pos < tb->cap-1) {
			memmove(&tb->text[b->c_pos+1], &tb->text[b->c_pos], sizeof(int)*(tb->cap - b->c_pos-1));
			tb->text[b->c_pos] = ch;
		   tb_textbox_move_cursor(tb, 1);
		}
	}
}


void tb_textbox_del_char(struct tb_widget *tb) {
	if (!tb) {
		return;
	}
	
	struct tb_textbox *b = tb->widget;
	if (b) {
		int len = tb_strlen(tb->text);
		if (b->c_pos < len) {
			tb->text[b->c_pos] = 0;
			memmove(&tb->text[b->c_pos], &tb->text[b->c_pos+1], sizeof(int)*(len - b->c_pos - 1));
			tb->text[len-1] = 0;
		}
	}
}

void tb_textbox_backspace(struct tb_widget *tb) {
	if (!tb) {
		return;
	}

	struct tb_textbox *b = tb->widget;
	if (b) {
		if (b->c_pos < tb->cap && b->c_pos > 0) {
			memmove(&tb->text[b->c_pos-1], &tb->text[b->c_pos], sizeof(int)*(tb->cap - b->c_pos));
		
			tb->text[tb->cap-1] = 0;
			tb_textbox_move_cursor(tb, -1);
		}

	}
}
