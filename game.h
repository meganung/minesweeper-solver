#ifndef __BOARD_H__
#define __BOARD_H__
#include <stdlib.h> 

class Game {
private:
    int width;
    int height;
    int numMines;
    int** mines; //2d list numMines x 2 that stores the coordinates of the mines
    int** board; //2d list width x height storing the mines
    int** playboard; // 2d list width x height
public:
    Game(int w, int h, int n);
    void setMines();
    void printBoard();
};

#endif