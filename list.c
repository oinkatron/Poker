#include <stdlib.h>
#include <string.h>

#include "list.h"

struct List* List_Create() {
	struct List *l = malloc(sizeof(struct List));
	memset(l, 0, sizeof(struct List));

	return l;
}

void List_Free(struct List *l) {
	if(l) {
		List_Clear(l);
		free(l);
	}
}

int List_PushHead(struct List *l, void *data) {
	if (!l) {
		return 0;
	}

	if (l->size == 0) { //Empty list
		++l->size;
		l->head = (struct L_node*)malloc(sizeof(struct L_node));
		l->tail = l->head;

		l->head->data = data;
		l->head->prev = 0;
		l->head->next = 0;

		return 1;
	}

	struct L_node *n = (struct L_node*)malloc(sizeof(struct L_node));
	struct L_node *tmp = l->head;
	
	l->head = n;
	n->next = tmp;
	n->prev = 0;
	n->data = data;
	tmp->prev = n;
	++l->size;
	
	return 1;
}

int List_Push(struct List *l, void *data) {
	if (!l) {
		return 0;
	}

	if (l->size == 0) { //Empty list
		l->head = (struct L_node*)malloc(sizeof(struct L_node));
		l->tail = l->head;

		l->head->data = data;
		l->head->prev = 0;
		l->head->next = 0;

		++l->size;
		return 1;
	}

	struct L_node *n = (struct L_node*)malloc(sizeof(struct L_node));
	struct L_node *tmp = l->tail;
	
	l->tail = n;
	n->prev = tmp;
	n->next = 0;
	n->data = data;
	tmp->next = n;

	++l->size;
	
	return 1;
}

int List_Insert(struct List *l, unsigned int ind, void *data) {
	if (!l) {
		return 0;
	}

	if (ind == 0) { //insert at head
		return List_PushHead(l, data);
	} else if (ind == l->size) { //insert at tail
		return List_Push(l, data);
	} else if (ind > l->size) { //out or range bugger off
		return 0;
	}

	unsigned int cnt = 1;
	struct L_node *c = l->head->next;
	while (c) {
		if (cnt == ind) { //insert here
			struct L_node *n = (struct L_node*)malloc(sizeof(struct L_node));
			//reorder dem nodes
			n->prev = c->prev;
			n->prev->next = n;
			n->next = c;
			n->data = data;
			c->prev = n;

			return 1;	
		}

		++cnt;
		c = c->next;
	}

	return 0;

}

struct L_node* List_GetNode(struct List *l, unsigned int ind) {
	if (!l || ind >= l->size) {
		return NULL;
	}

	if (ind == 0) {
		return l->head;
	} else if (ind == l->size-1) {
		return l->tail;
	}

	int cnt=1;
	for (struct L_node *n = l->head->next; n != NULL; n = n->next) {
		if (cnt == ind) {
			return n;
		}
		cnt++;
	}

	return NULL;

}


void* List_Get(struct List *l, unsigned int ind) {
	if (!l) {
		return 0;
	}

	if (ind == 0) { //grab head
		return l->head->data;
	} else if (ind == l->size-1) { //grab tail
		return l->tail->data;
	} else if (ind >= l->size) { //out or range bugger off
		return 0;
	}

	unsigned int cnt = 1;
	for (struct L_node *c = l->head->next; c != 0; c = c->next) {
		if (cnt == ind) { //insert here
			return c->data;
		}

		++cnt;
	}

	return 0;
}

int List_Remove(struct List *l, unsigned int ind) {
	if (!l) {
		return 0;
	}
	
	struct L_node *c=0;

	if (ind == 0) { //remove head
		c = l->head;
		l->head = l->head->next;
		l->head->prev = 0;
		
		--l->size;
		
		free(c);		

		return 1;
	} else if (ind == l->size-1) { //remove at tail
		c = l->tail;
		l->tail = l->tail->prev;
		l->tail->next = 0;

		--l->size;
		free(c);
		
		return 1;
	} else if (ind >= l->size) { //out or range bugger off
		return 0;
	}

	unsigned int cnt = 1;
	for (c = l->head->next; c != 0; c = c->next) {
		if (cnt == ind) { //remove here
			c->prev->next = c->next;
			if (c->next)
				c->next->prev = c->prev;
			
			--l->size;
			free(c);	
			
			return 1;
		}

		++cnt;
	}

	return 0;
}

int List_Clear(struct List *l) {
	if (!l || l->size == 0) {
		return 1;
	}
	
	struct L_node *nxt=0;
	for (struct L_node *c = l->head; c != 0; c = nxt) {
		nxt = c->next;
		free(c);
	}

	l->size=0;
	l->head=0;
	l->tail=0;
	return 1;
}
