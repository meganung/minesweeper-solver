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
    int runseq = 0;
    int runpar = 1;
    int runomp = 0;
    int mode = -1;
    int test = 0;
    int opt;
    
    while ((opt = getopt(argc, argv, "h:w:n:m:opat")) != -1) {
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
            case 'a':
                runseq = 1;
                runpar = 1;
                runomp = 1;
                break;
            case 'o':
                runomp = 1;
                runseq = 0;
                runpar = 0;
                break;
            case 'p':
                print = 1;
                break;
            case 't':
                //tests
                test = 1;
                runseq = 0;
                runpar = 0;
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
    double partime = 0;
    double seqtime = 0;
    double omptime = 0;
    if (runseq) {
        //SEQUENTIAL
        // printf("starting SEQUENTIAL\n");
        seqtime = game->seqSolve(1);
        assert(game->resultCheck());
        if (print) {
            printf("Mines found: ");
            for (int i = 0; i < game->numMines; i++) {
                printf("(%d, %d), ",game->playmines[i*2], game->playmines[i*2+1]);
            }
            printf("\n");
        }
        printf("Sequential Overall: %.3f ms\t\t\n", 1000.f * seqtime);

    }
    game->clearPlayboards();

    if (runpar) {
        //PARALLEL
        // printf("starting PARALLEL\n");
        
        partime = game->parSolve(1);
        assert(game->resultCheck());
        if (print) {
            printf("Mines found: ");
            for (int i = 0; i < game->numMines; i++) {
                printf("(%d, %d), ",game->playmines[i*2], game->playmines[i*2+1]);
            }
            printf("\n");
        }
        printf("Cuda Overall: %.3f ms\t\t\n", 1000.f * partime);

    }

    game->clearPlayboards();

    if (runomp && !test) {
        //PARALLEL OPEN MP
        omptime = game -> ompSolve(1);
        assert(game->resultCheck());
        printf("OpenMP Overall: %.3f ms\t\t\n", 1000.f * omptime);
    }

    game->clearPlayboards();


    if (runpar && runseq) {
        printf("TIMING\n");
        printf("sequential: %.3f ms\t\t cuda: %.3f ms\n",1000.f * seqtime,1000.f * partime);
        printf("speedup: %.3f\n", seqtime / partime);
    }
    

    // if (test && !runomp) {
    //     int num = 100;
    //     double numsuccess = 0;
    //     double seqsum = 0;
    //     double parsum = 0;
    //     int seqerr = 0;
    //     int parerr = 0;
    //     double speedupsum = 0;
    //     for (int i = 0; i < num; i++) {
    //         double iseq = game->seqSolve();
    //         bool seqok = game->resultCheck();
    //         game->clearPlayboards();
            
    //         double ipar = game->parSolve();
    //         bool parok = game->resultCheck();
    //         game->clearPlayboards();

    //         if (!seqok) {
    //             seqerr++;
    //         } 
    //         if (!parok) {
    //             parerr++;
    //         }
    //         if (parok && seqok) {
    //             numsuccess++;
    //             seqsum += iseq;
    //             parsum += ipar;
    //             speedupsum += iseq / ipar;
    //             printf("%d ok: seq %0.3f ms \t cuda %0.3f ms \t speedup: %0.3f \n", i, iseq * 1000.f, ipar * 1000.f, iseq/ipar);
    //         } else {
    //             printf("%d notok\n",i);
    //         }
    //     }
    //     printf("SUMMARY: \n");
    //     printf("avg cuda: %0.3f ms\n",parsum * 1000.f / numsuccess);
    //     printf("avg sequential: %0.3f ms\n",seqsum * 1000.f / numsuccess);
    //     printf("avg speedup: %0.3f \n",speedupsum / numsuccess);
    // }

    if (test) {
        int num = 100;
        // double numsuccess = 0;
        double seqsum = 0;
        double parsum = 0;
        double ompsum = 0;
        double sharedsum = 0;
        int seqok = 0;
        int parok = 0;
        int ompok = 0;
        int sharedok = 0;
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

                double ishared = game->sharedParSolve(i+1);
                bool sharedres = game->resultCheck();
                game->clearPlayboards();

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
                if (sharedres) {
                    sharedok++;
                    sharedsum += ishared;
                }
                // printf("%d: seq %0.3f ms \t cuda %0.3f ms \t shared %0.3f ms \t omp %.3f ms \n", i, iseq * 1000.f * seqres, ipar * 1000.f * parres, ishared * 1000.f * sharedres, iomp * 1000.f * ompres);
                
            }
            printf("SUMMARY: \n");
            printf("avg sequential: %0.3f ms\n",seqsum * 1000.f / seqok);
            printf("avg cuda: %0.3f ms\n",parsum * 1000.f / parok);
            printf("avg shared: %0.3f ms\n",sharedsum * 1000.f / sharedok);
            printf("avg omp: %0.3f ms\n",ompsum * 1000.f / ompok);
            printf("SPEEDUP: cuda: %0.3f \t shared: %0.3f \t omp: %0.3f \n", (seqsum * 1000.f / seqok) / (parsum * 1000.f / parok), (seqsum * 1000.f / seqok) / (sharedsum * 1000.f / sharedok), (seqsum * 1000.f / seqok) / (ompsum * 1000.f / ompok) );
            printf("num success: seq: %d \t cuda: %d \t shared: %d \t omp: %d\n", seqok, parok, sharedok, ompok);
        }
    }
    

    // printf("MY MINES I FOUND: \n");

    
    // for (int i = 0 ; i < 2*game->numMines; i+=2){
    //     printf("%d %d\n",game->parplaymines[i],game->parplaymines[i+1]);
    // }
    // printf("playboard\n");
    // game->printBoard(game->playboard);


    

    return 0;
}