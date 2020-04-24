#include "game.h"
#include <ctime>
#include <stdio.h> 

//game constructor
Game::Game(int w, int h, int n) {
    width = w;
    height = h;
    numMines = n;
    playminecount = 0;
    mines = (int**)malloc(numMines * sizeof(int*));
    parboard = (int*)calloc(height*width, sizeof(int));
    parplayboard = (int*)calloc(height*width, sizeof(int));
    for (int i = 0; i < numMines; i++) {
        mines[i] = (int*)calloc(2, sizeof(int));
    }
    board = (int**)malloc(height * sizeof(int*));
    playboard = (int**)malloc(height * sizeof(int*));
    for (int i = 0; i < height; i++) {
        board[i] = (int*)calloc(width, sizeof(int));
        playboard[i] = (int*)calloc(width, sizeof(int));
    }
    playmines = (int*)calloc(numMines * 2, sizeof(int));
    parplaymines = (int*)calloc(numMines * 2, sizeof(int));

}


float Game::toBW(int bytes, float sec) {
    return static_cast<float>(bytes) / (1024. * 1024. * 1024.) / sec;
  }

void Game::setMines() {
    srand(time(NULL));
    for (int i = 0; i < numMines; i++) {
        int x = rand() % height;
        int y = rand() % width;
        while (board[x][y] == -1) {
            x = rand() % width;
            y = rand() % height;
        }
        mines[i][0] = x;
        mines[i][1] = y;
        board[x][y] = -1;
        parboard[x * width + y] = -1;
        //populate the number hints around
        for (int i = -1; i < 2; i++) {
            for (int j = -1; j < 2; j++) {
                int xi = x+i;
                int yi = y+j;
                if (xi >= 0 && xi < height && yi >= 0 && yi < width && !(i == 0 && j == 0) && board[xi][yi] != -1) {
                    board[xi][yi] += 1;
                    parboard[xi * width + yi] += 1;
                }
            }
        }
    }
}

void Game::printBoard(int** b) {
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (b[i][j] < 0) {
                printf("* ");
            } else {
                printf("%d ",b[i][j]);
            }
        }
        printf("\n");
    }
}

bool Game::resultCheck(int x) {
    //TODO: check playminecount? 
    // need to use playminecount in the parallel version then
    int* minearr = playmines;
    if (x) {
        //parallel
        minearr = parplaymines;
    }
    for (int i = 0; i < numMines*2; i+=2){
        if (board[minearr[i]][minearr[i+1]] != -1) {
            printf("RESULT CHECK FAILED: %d %d, %d\n",minearr[i],minearr[i+1],i);
            return false;
        }
    }
    return true;
        

}