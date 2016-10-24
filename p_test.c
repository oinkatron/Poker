#include "poker.h"

int main() {
	Deck *d = Deck_Create(10);

	for (int i =0; i<20; i++) {
		printCard(Deck_DealCard(d));
	}

	return 0;
}
