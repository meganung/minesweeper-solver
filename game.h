#ifndef __BOARD_H__
#define __BOARD_H__
#include <stdlib.h> 
#include <tuple>
#include <list>

using namespace std; 

class Game {
public:
    int width;
    int height;
    int numMines;
    int** mines; //2d list numMines x 2 that stores the coordinates of the mines
    Game(int w, int h, int n);
    int** board; //2d list width x height storing the mines
    int** playboard; // 2d list width x height
    list<tuple<int,int>> safeMoves;
    list<tuple<int,int>> playmines;
    int totalrevealed;
    void setMines();
    void printBoard(int** b);
    void seqSolve();
    int checkNeighbors(int x, int y);
    tuple<int,int> chooseRandomMove();
};

#endif