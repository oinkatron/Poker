#ifndef LIST_H
#define LIST_H

struct L_node {
	void *data;
   struct L_node *next;
	struct L_node *prev;
};

struct List {
	unsigned short size;

	struct L_node *head;
	struct L_node *tail;

};

struct List* List_Create();
void List_Free(struct List *l);

int List_PushHead(struct List *l, void *data);
int List_Push(struct List *l, void *data);
int List_Insert(struct List *l, unsigned int ind,  void *data);

void* List_Get(struct List *l, unsigned int ind);
struct L_node*  List_GetNode(struct List *l, unsigned int ind);

int List_Remove(struct List *l, unsigned int ind);
int List_Clear(struct List *l);

#endif
