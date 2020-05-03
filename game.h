#ifndef __BOARD_H__
#define __BOARD_H__
#include <stdlib.h> 
#include <tuple>
#include <list>
#include <cassert>

using namespace std; 

class Game {
public:
    int width;
    int height;
    int numMines;
    int** mines; //2d list numMines x 2 that stores the coordinates of the mines
    int** board; //2d list width x height storing the mines
    int** playboard; // 2d list width x height
    int* parboard;
    int* parplayboard;
    int* playmines; // 1d list size numMines x 2 
    int* parplaymines; 
    int playminecount;
    Game(int w, int h, int n);
    void printBoard(int** b);
    void setMines();
    int countAdjMines(int x, int y);
    int countUnrevealed(int x, int y);
    void markNeighbors(int x, int y);
    void revealNeighbors(int x, int y);
    tuple<int,int> chooseRandomMove();
    double seqSolve();
    void parSolve();
    double sharedParSolve();
    bool resultCheck(int x);

    float toBW(int bytes, float sec);
};

#endif