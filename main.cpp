#include "game.h"
#include <stdlib.h> 
#include <stdio.h> 

using namespace std;

int main() {
    Game* game = new Game(400,400,75);
    game->setMines();
    // game->printBoard(game->board);
    printf("starting\n");
    game->parplaymines = (int*)malloc(sizeof(int)*2*game->numMines);
    // game->parSolve();
    // printf("MY MINES I FOUND: \n");

    
    // for (int i = 0 ; i < 2*game->numMines; i+=2){
    //     printf("%d %d\n",game->parplaymines[i],game->parplaymines[i+1]);
    // }
    // printf("playboard\n");
    // game->printBoard(game->playboard);
    // assert(game->parResultCheck());


    //SEQUENTIAL
    game->seqSolve();
    // printf("MY MINES I FOUND: \n");

    // for (auto const& i: game->playmines) {
    //     int x = get<0>(i);
    //     int y = get<1>(i);
    //     printf("%d, %d\n",x,y);
	// }
    return 0;
}