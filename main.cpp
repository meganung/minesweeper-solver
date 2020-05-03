#include "game.h"
#include <stdlib.h> 
#include <stdio.h> 
#include <unistd.h>

using namespace std;

int main(int argc,char* argv[]) {
    int height = 100;
    int width = 100;
    int nummines = 30;
    int print = 0;
    int runseq = 0;
    int runpar = 1;
    int opt;
    
    while ((opt = getopt(argc, argv, "h:w:n:psa")) != -1) {
        switch (opt) {
            case 'h':                   
                height = atoi(optarg);
                break;
            case 'w':                   
                width = atoi(optarg);
                break;
            case 'n':                   
                nummines = atoi(optarg);
                break;
            case 's':
                runseq = 1;
                runpar = 0;
                break;
            case 'a':
                runseq = 1;
                runpar = 1;
                break;
            case 'p':
                print = 1;
                break;
            default:
                printf("Usage: ./cudaGame -h h -w w -n n -s -a -p\n");
                return -1;
        }
    }


    Game* game = new Game(width,height,nummines);
    game->setMines();
    printf("starting\n");
    if (print) {
        game->printBoard(game->board);
    }
    if (runseq) {
        //SEQUENTIAL
        printf("SEQUENTIAL\n");
        game->seqSolve();
        assert(game->resultCheck(0));
        if (print) {
            printf("Mines found: ");
            for (int i = 0; i < game->numMines; i++) {
                printf("(%d, %d), ",game->playmines[i*2], game->playmines[i*2+1]);
            }
            printf("\n");
        }
    }
    if (runpar) {
        //PARALLEL
        printf("PARALLEL\n");
        
        game->sharedParSolve();
        assert(game->resultCheck(1));

    }
    // printf("MY MINES I FOUND: \n");

    
    // for (int i = 0 ; i < 2*game->numMines; i+=2){
    //     printf("%d %d\n",game->parplaymines[i],game->parplaymines[i+1]);
    // }
    // printf("playboard\n");
    // game->printBoard(game->playboard);


    

    return 0;
}