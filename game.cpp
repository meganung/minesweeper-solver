#include "game.h"
#include <ctime>
#include <stdio.h> 

//game constructor
Game::Game(int w, int h, int n) {
    width = w;
    height = h;
    numMines = n;
    mines = (int**)malloc(numMines * sizeof(int*));
    for (int i = 0; i < numMines; i++) {
        mines[i] = (int*)calloc(2, sizeof(int));
    }
    board = (int**)malloc(width * sizeof(int*));
    playboard = (int**)malloc(width * sizeof(int*));
    for (int i = 0; i < height; i++) {
        board[i] = (int*)calloc(height, sizeof(int));
        playboard[i] = (int*)calloc(height, sizeof(int));
    }
}

void Game::setMines() {
    srand(time(NULL));
    for (int i = 0; i < numMines; i++) {
        int x = rand() % width;
        int y = rand() % height;
        mines[i][0] = x;
        mines[i][1] = y;
        board[x][y] = -1;
    }
}

void Game::printBoard() {
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            printf("%d ",board[i][j]);
        }
        printf("\n");
    }
}
