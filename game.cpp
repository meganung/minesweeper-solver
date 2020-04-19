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
        while (board[x][y] == -1) {
            x = rand() % width;
            y = rand() % height;
        }
        mines[i][0] = x;
        mines[i][1] = y;
        board[x][y] = -1;
        //populate the number hints around
        for (int i = -1; i < 2; i++) {
            for (int j = -1; j < 2; j++) {
                int xi = x+i;
                int yi = y+j;
                if (xi >= 0 && xi < height && yi >= 0 && yi < width && !(i == 0 && j == 0) && board[xi][yi] != -1) {
                    board[xi][yi] += 1;
                }
            }
        }
    }
}

void Game::printBoard() {
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (board[i][j] < 0) {
                printf("* ");
            } else {
                printf("%d ",board[i][j]);
            }
        }
        printf("\n");
    }
}
