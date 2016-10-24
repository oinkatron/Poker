#ifndef POKER_H
#define POKER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef unsigned char Card;

#define DECK_SIZE 52

#define CARD_INVALID_VALUE 13
#define CARD_INVALUD_SUIT 12

#define SUIT_HEARTS 0
#define SUIT_CLUBS 1
#define SUIT_DIAMONDS 2
#define SUIT_SPADES 3

int8_t cardValue(Card c);
int8_t cardSuit(Card c);

void printCard(Card c);

typedef struct { 
	uint64_t seed; 
	uint8_t index; 
	Card cards[DECK_SIZE]; 
} Deck;

Deck* Deck_Create(uint64_t seed);
void Deck_Shuffle(Deck *d);
Card Deck_DealCard(Deck *d);

#endif 
