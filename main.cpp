#include "game.h"
#include <stdlib.h> 
#include <stdio.h> 

using namespace std;

int main() {
    Game* game = new Game(8,8,3);
    game->setMines();
    game->printBoard();
    return 0;
}