#include "game.h"
#include <list> 
#include <string>
#include <tuple>
#include "CycleTimer.h"

//using namespace Game;
using namespace std;

tuple<int,int> Game::chooseRandomMove() {
    int xinit = rand() % height;
    int yinit = rand() % width;
    while (playboard[xinit][yinit] == 1) {
        xinit = rand() % height;
        yinit = rand() % width;
    }
    return make_tuple(xinit,yinit);
}

//count uncovered adj mines (meaning already marked)
int Game::countAdjMines(int x, int y) {
    int c = 0;
    for (int i = -1; i < 2; i++) {
        for (int j = -1; j < 2; j++) {
            int xi = x+i;
            int yi = y+j;
            if (xi >= 0 && xi < height && yi >= 0 && yi < width && !(i == 0 && j == 0)) {
                if (playboard[xi][yi] == 1 && board[xi][yi] == -1) {
                    c++;
                } 
            }
        }
    } 
    return c;
}

//reveal neighbors not revealed yet
void Game::revealNeighbors(int x, int y) {
    for (int i = -1; i < 2; i++) {
        for (int j = -1; j < 2; j++) {
            int xi = x+i;
            int yi = y+j;
            if (xi >= 0 && xi < height && yi >= 0 && yi < width && !(i == 0 && j == 0)) {
                if (playboard[xi][yi] == 0) {
                    playboard[xi][yi] = 1;
                    if (board[xi][yi] == -1) {
                        printf("DID A BAD\n");
                    }
                } 
            }
        }
    } 
}

int Game::countUnrevealed(int x, int y) {
    int c = 0;
    for (int i = -1; i < 2; i++) {
        for (int j = -1; j < 2; j++) {
            int xi = x+i;
            int yi = y+j;
            if (xi >= 0 && xi < height && yi >= 0 && yi < width && !(i == 0 && j == 0)) {
                if (playboard[xi][yi] == 0) {
                    c++;
                } 
            }
        }
    } 
    return c;
}
void Game::markNeighbors(int x, int y) {
    for (int i = -1; i < 2; i++) {
        for (int j = -1; j < 2; j++) {
            int xi = x+i;
            int yi = y+j;
            if (xi >= 0 && xi < height && yi >= 0 && yi < width && !(i == 0 && j == 0)) {
                if (playboard[xi][yi] == 0) {
                    playboard[xi][yi] = 1;
                    playmines[playminecount * 2] = xi;
                    playmines[playminecount * 2 + 1] = yi;
                    playminecount++;
                }
            }
        }
    } 
    
}

double Game::seqSolve(int iter) {
    // int totalBytes = sizeof(int) * height * width;
    double startTime = CycleTimer::currentSeconds();
    srand(time(0) * iter);
    int guesses = 0;  
    while(playminecount < numMines) {
        tuple<int,int> rmove = chooseRandomMove();
        int x = get<0>(rmove);
        int y = get<1>(rmove);
        guesses++;
        if (board[x][y] == -1) {
            // printf("\n");
            // printf("oops %dth guess was a bomb big sad\n",guesses);
            return -1;
        } else {
            //reveal
            playboard[x][y] = 1;
        }
        bool progress = true;
        while (progress) {
            progress = false;
            for (int i = 0; i < height; i++) {
                for (int j = 0; j < width; j++) {
                    if (playboard[i][j] == 1 && board[i][j] != -1 ) { //clear square
                        int adjmines = countAdjMines(i,j);
                        int unrevealed = countUnrevealed(i,j);
                        if (unrevealed != 0 ){
                            if (adjmines == board[i][j]) { //all mines found
                                //reveal neighbors
                                progress = true;
                                revealNeighbors(i,j);
                            }
                            if (unrevealed == board[i][j] - adjmines && unrevealed >= 0) {
                                progress = true;
                                markNeighbors(i,j);
                            }
                        }
                        
                    }
                }
            }
        }
    }

    double endTime = CycleTimer::currentSeconds();


    double overallDuration = endTime - startTime;
    // printf("Overall: %.3f ms\t\t[%.3f GB/s]\n", 1000.f * overallDuration, toBW(totalBytes, overallDuration));
    return overallDuration;
}