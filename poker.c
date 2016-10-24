#include "poker.h"

int8_t cardValue(Card c) {
	if (c > 51) {
		return CARD_INVALID_VALUE;		
	}

	return (c >> 2);
}

int8_t cardSuit(Card c) {
	return (c & 0x03);
}

void printCard(Card c) {
	int8_t val = cardValue(c);
	switch(val) {
		case 0:
			printf("Ace of ");
			break;
		case 10:
			printf("Jack of ");
			break;
		case 11:
			printf("Queen of ");
			break;
		case 12:
			printf("King of ");
			break;
		default:
			printf("%d of ", val+1);
	}
	switch(cardSuit(c)) {
		case SUIT_HEARTS:
			printf("hearts\n");
			break;
		case SUIT_CLUBS:
			printf("clubs\n");
			break;
		case SUIT_DIAMONDS:
			printf("diamonds\n");
			break;
		case SUIT_SPADES:
			printf("spades\n");
			break;
		
	}
}

Deck* Deck_Create(uint64_t seed) {
	Deck *d = malloc(sizeof(Deck));
	d->seed = seed;
	d->index = 0;

	for (int i=0; i < DECK_SIZE; i++) {
		d->cards[i] = i;
	}

	srand(seed);
	Deck_Shuffle(d);

	return d;
}

void Deck_Shuffle(Deck *d) {
	int j = 0;
	Card tmp;
	for(int i=0; i < DECK_SIZE-1; ++i) {
		j = i + (rand() % (DECK_SIZE - i));
		tmp = d->cards[i];
		d->cards[i] = d->cards[j];
		d->cards[j] = tmp;
	}
}

Card Deck_DealCard(Deck *d) {
	if (d->index >= DECK_SIZE) {
		d->index = 0;
	}
	
	return d->cards[d->index++];
}
