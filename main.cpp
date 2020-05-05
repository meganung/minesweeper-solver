#include "game.h"
#include <stdlib.h> 
#include <stdio.h> 
#include <unistd.h>

using namespace std;

int main(int argc,char* argv[]) {
    int height = 100;
    int width = 100;
    int nummines = 250;
    int print = 0;
    int mode = -1;
    int test = 0;
    int opt;
    
    while ((opt = getopt(argc, argv, "h:w:n:m:pt")) != -1) {
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
            case 'm':
                mode = atoi(optarg);
                break;
            case 'p':
                print = 1;
                break;
            case 't':
                //tests
                test = 1;
                break;
            default:
                printf("Usage: ./minesweeper -h height -w width -n numMines -m 0,1,2,3 (seq, cuda, cuda w shared mem, openmp) -p (print) -t (testing) \n");
                return -1;
        }
    }


    Game* game = new Game(width,height,nummines);
    game->setMines();
    printf("starting\n");
    if (print) {
        game->printBoard(game->board);
    }


    if (test) {
        int num = 100;
        double seqsum = 0;
        double parsum = 0;
        double ompsum = 0;
        // double sharedsum = 0;
        int seqok = 0;
        int parok = 0;
        int ompok = 0;
        // int sharedok = 0;
        if (mode == 0) {
            printf("TEST SEQUENTIAL\n");
            for (int i = 0; i < num; i++) {
                double iseq = game->seqSolve(i+1);
                bool seqres = game->resultCheck();
                game->clearPlayboards();
                if (seqres) {
                    seqok++;
                    seqsum += iseq;
                } 
                printf("%d: seq %0.3f ms \n", i, iseq * 1000.f * seqres);

            }
            printf("SUMMARY: \n");
            printf("avg sequential: %0.3f ms\n",seqsum * 1000.f / seqok);
            printf("num success: seq: %d \n", seqok);
        }
        else if (mode == 1) {
            printf("TEST CUDA, not shared mem\n");
            for (int i = 0; i < num; i++) {
                double iseq = game->seqSolve(i+1);
                bool seqres = game->resultCheck();
                game->clearPlayboards();

                double ipar = game->parSolve(i+1);
                bool parres = game->resultCheck();
                game->clearPlayboards();

                if (seqres) {
                    seqok++;
                    seqsum += iseq;
                } 
                if (parres) {
                    parok++;
                    parsum += ipar;
                }
                // printf("%d: seq %0.3f ms \t cuda %0.3f ms \n", i, iseq * 1000.f * seqres, ipar * 1000.f * parres);

            }
            printf("SUMMARY: \n");
            printf("avg sequential: %0.3f ms\n",seqsum * 1000.f / seqok);
            printf("avg cuda: %0.3f ms\n",parsum * 1000.f / parok);
            printf("SPEEDUP: %0.3f \n", (seqsum * 1000.f / seqok) /(parsum * 1000.f / parok));
            printf("num success: seq: %d \t cuda: %d\n", seqok, parok);
        } else if (mode == 2) {
            printf("TEST CUDA SHARED MEM\n");
            for (int i = 0; i < num; i++) {
                double iseq = game->seqSolve(i+1);
                bool seqres = game->resultCheck();
                game->clearPlayboards();

                double ipar = game->sharedParSolve(i+1);
                bool parres = game->resultCheck();
                game->clearPlayboards();

                if (seqres) {
                    seqok++;
                    seqsum += iseq;
                } 
                if (parres) {
                    parok++;
                    parsum += ipar;
                }
                // printf("%d: seq %0.3f ms \t cuda shared %0.3f ms \n", i, iseq * 1000.f * seqres, ipar * 1000.f * parres);

            }
            printf("SUMMARY: \n");
            printf("avg sequential: %0.3f ms\n",seqsum * 1000.f / seqok);
            printf("avg cuda shared: %0.3f ms\n",parsum * 1000.f / parok);
            printf("SPEEDUP: %0.3f \n", (seqsum * 1000.f / seqok) /(parsum * 1000.f / parok));
            printf("num success: seq: %d \t cuda shared: %d\n", seqok, parok);
        } else if (mode == 1) {
            printf("TEST OPENMP\n");
            for (int i = 0; i < num; i++) {
                double iseq = game->seqSolve(i+1);
                bool seqres = game->resultCheck();
                game->clearPlayboards();

                double ipar = game->ompSolve(i+1);
                bool parres = game->resultCheck();
                game->clearPlayboards();

                if (seqres) {
                    seqok++;
                    seqsum += iseq;
                } 
                if (parres) {
                    parok++;
                    parsum += ipar;
                }
                // printf("%d: seq %0.3f ms \t omp %0.3f ms \n", i, iseq * 1000.f * seqres, ipar * 1000.f * parres);

            }
            printf("SUMMARY: \n");
            printf("avg sequential: %0.3f ms\n",seqsum * 1000.f / seqok);
            printf("avg omp: %0.3f ms\n",parsum * 1000.f / parok);
            printf("SPEEDUP: %0.3f \n", (seqsum * 1000.f / seqok) /(parsum * 1000.f / parok));
            printf("num success: seq: %d \t omp: %d\n", seqok, parok);
        } else {
            for (int i = 0; i < num; i++) {
                double iseq = game->seqSolve(i+1);
                bool seqres = game->resultCheck();
                game->clearPlayboards();

                double ipar = game->parSolve(i+1);
                bool parres = game->resultCheck();
                game->clearPlayboards();

                double iomp = game->ompSolve(i+1);
                bool ompres = game->resultCheck();
                game->clearPlayboards();

                // double ishared = game->sharedParSolve(i+1);
                // bool sharedres = game->resultCheck();
                // game->clearPlayboards();

                if (seqres) {
                    seqok++;
                    seqsum += iseq;
                } 
                if (parres) {
                    parok++;
                    parsum += ipar;
                }
                if (ompres) {
                    ompok++;
                    ompsum += iomp;
                }
                // if (sharedres) {
                //     sharedok++;
                //     sharedsum += ishared;
                // }
                // printf("%d: seq %0.3f ms \t cuda %0.3f ms \t shared %0.3f ms \t omp %.3f ms \n", i, iseq * 1000.f * seqres, ipar * 1000.f * parres, ishared * 1000.f * sharedres, iomp * 1000.f * ompres);
                
            }
            printf("SUMMARY: \n");
            printf("avg sequential: %0.3f ms\n",seqsum * 1000.f / seqok);
            printf("avg cuda: %0.3f ms\n",parsum * 1000.f / parok);
            // printf("avg shared: %0.3f ms\n",sharedsum * 1000.f / sharedok);
            printf("avg omp: %0.3f ms\n",ompsum * 1000.f / ompok);
            // printf("SPEEDUP: cuda: %0.3f \t shared: %0.3f \t omp: %0.3f \n", (seqsum * 1000.f / seqok) / (parsum * 1000.f / parok), (seqsum * 1000.f / seqok) / (sharedsum * 1000.f / sharedok), (seqsum * 1000.f / seqok) / (ompsum * 1000.f / ompok) );
            printf("SPEEDUP: cuda: %0.3f \t omp: %0.3f \n", (seqsum * 1000.f / seqok) / (parsum * 1000.f / parok), (seqsum * 1000.f / seqok) / (ompsum * 1000.f / ompok) );
            printf("num success: seq: %d \t cuda: %d \t omp: %d\n", seqok, parok, ompok);
        }
    } else {
        if (mode == 1) {
            game->clearPlayboards();
            double iseq = game->seqSolve(2);
            bool seqres = game->resultCheck();
            game->clearPlayboards();

            double ipar = game->parSolve(2);
            bool parres = game->resultCheck();

            if (seqres && parres) {
                printf("seq %0.3f ms \t cudam %0.3f ms \t SPEEDUP: %0.3f \n", iseq * 1000.f * seqres, ipar * 1000.f * parres, (iseq * 1000.f * seqres) / (ipar * 1000.f * parres));
            } else {
                printf("seq %d or par %d failed\n",seqres,parres);
            }
        } else if (mode == 2) {
            game->clearPlayboards();
            double iseq = game->seqSolve(2);
            bool seqres = game->resultCheck();
            game->clearPlayboards();

            double ipar = game->sharedParSolve(2);
            bool parres = game->resultCheck();

            if (seqres && parres) {
                printf("seq %0.3f ms \t cuda shared mem %0.3f ms \t SPEEDUP: %0.3f \n", iseq * 1000.f * seqres, ipar * 1000.f * parres, (iseq * 1000.f * seqres) / (ipar * 1000.f * parres));
            } else {
                printf("seq %d or par %d failed\n",seqres,parres);
            }
        } else if (mode == 3) {
            game->clearPlayboards();
            double iseq = game->seqSolve(2);
            bool seqres = game->resultCheck();
            game->clearPlayboards();

            double ipar = game->ompSolve(2);
            bool parres = game->resultCheck();

            if (seqres && parres) {
                printf("seq %0.3f ms \t omp %0.3f ms \t SPEEDUP: %0.3f \n", iseq * 1000.f * seqres, ipar * 1000.f * parres, (iseq * 1000.f * seqres) / (ipar * 1000.f * parres));
            } else {
                printf("seq %d or omp %d failed\n",seqres,parres);
            }
        } else {
            //sequential
            double iseq = game->seqSolve(2);
            bool seqres = game->resultCheck();
            if (seqres) {
                printf("Sequential time taken: %0.3f ms\n",iseq * 1000.f);
            } else {
                printf("seq failed (blew up a mine)\n");
            }
        }
    }
    
    if (print && !test) {
        printf("MINES FOUND: \n");
        for (int i = 0 ; i < 2*game->numMines; i+=2){
            printf("(%d, %d), ",game->playmines[i],game->playmines[i+1]);
        }
        printf("\n");
    }


  


    

    return 0;
}