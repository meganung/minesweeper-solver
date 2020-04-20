#include "game.h"
#include <stdlib.h> 
#include <stdio.h> 

using namespace std;

int main() {
    Game* game = new Game(8,8,5);
    game->setMines();
    game->printBoard(game->board);
    game->seqSolve();
    printf("MY MINES I FOUND: \n");
    for (auto const& i: game->playmines) {
        int x = get<0>(i);
        int y = get<1>(i);
        printf("%d, %d\n",x,y);
	}
    printf("playbaord\n");
    game->printBoard(game->playboard);
    return 0;
}