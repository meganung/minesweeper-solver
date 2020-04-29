#include "game.h"
#include <stdlib.h> 
#include <stdio.h> 
#include <unistd.h>

using namespace std;

int main(int argc,char* argv[]) {
    int height = 400;
    int width = 400;
    int nummines = 75;
    int print = 0;
    int runseq = 0;
    int runpar = 1;
    int runomp = 0;
    int test = 0;
    int opt;
    
    while ((opt = getopt(argc, argv, "h:w:n:opsat")) != -1) {
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
                runomp = 0;
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
        seqtime = game->seqSolve();
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
        
        partime = game->parSolve();
        assert(game->resultCheck());
        printf("Parallel Overall: %.3f ms\t\t\n", 1000.f * partime);

    }

    game->clearPlayboards();

    if (runomp && !test) {
        //PARALLEL OPEN MP
        omptime = game -> ompSolve();
        assert(game->resultCheck());
        printf("OpenMP Overall: %.3f ms\t\t\n", 1000.f * omptime);
    }

    game->clearPlayboards();


    if (runpar && runseq) {
        printf("TIMING\n");
        printf("sequential: %.3f ms\t\t parallel: %.3f ms\n",1000.f * seqtime,1000.f * partime);
        printf("speedup: %.3f\n", seqtime / partime);
    }
    

    if (test && !runomp) {
        int num = 5;
        double numsuccess = 0;
        double seqsum = 0;
        double parsum = 0;
        int seqerr = 0;
        int parerr = 0;
        double speedupsum = 0;
        for (int i = 0; i < num; i++) {
            double iseq = game->seqSolve();
            double ipar = game->parSolve();
            bool seqok = game->resultCheck();
            bool parok = game->resultCheck();

            if (!seqok) {
                seqerr++;
            } 
            if (!parok) {
                parerr++;
            }
            if (parok && seqok) {
                numsuccess++;
                seqsum += iseq;
                parsum += ipar;
                speedupsum += iseq / ipar;
                printf("%d ok: seq %0.3f ms \t par %0.3f ms \t speedup: %0.3f \n", i, iseq * 1000.f, ipar * 1000.f, iseq/ipar);
            }
            game->clearPlayboards();
        }
        printf("SUMMARY: \n");
        printf("avg parallel: %0.3f ms\n",parsum * 1000.f / numsuccess);
        printf("avg sequential: %0.3f ms\n",seqsum * 1000.f / numsuccess);
        printf("avg speedup: %0.3f \n",speedupsum / numsuccess);
    }

    if (test && runomp) {
        int num = 5;
        double numsuccess = 0;
        double seqsum = 0;
        double parsum = 0;
        double ompsum = 0;
        int seqerr = 0;
        int parerr = 0;
        int omperr = 0;
        double speedupsum = 0;
        for (int i = 0; i < num; i++) {
            double iseq = game->seqSolve();
            bool seqok = game->resultCheck();
            game->clearPlayboards();

            double ipar = game->parSolve();
            bool parok = game->resultCheck();
            game->clearPlayboards();

            double iomp = game->ompSolve();
            bool ompok = game->resultCheck();

            if (!seqok) {
                seqerr++;
            } 
            if (!parok) {
                parerr++;
            }
            if (!ompok) {
                omperr++;
            }
            if (parok && seqok && ompok) {
                numsuccess++;
                seqsum += iseq;
                parsum += ipar;
                ompsum += iomp;
                speedupsum += iseq / iomp;
                printf("%d ok: seq %0.3f ms \t par %0.3f ms \t omp %.3f ms \t seq/omp speedup: %0.3f \n", i, iseq * 1000.f, ipar * 1000.f, iomp * 1000.f, iseq/iomp);
            }
            game->clearPlayboards();
        }
        printf("SUMMARY: \n");
        printf("avg parallel: %0.3f ms\n",parsum * 1000.f / numsuccess);
        printf("avg sequential: %0.3f ms\n",seqsum * 1000.f / numsuccess);
        printf("avg omp: %0.3f ms\n",ompsum * 1000.f / numsuccess);
        printf("avg speedup: %0.3f \n",speedupsum / numsuccess);
    }
    

    // printf("MY MINES I FOUND: \n");

    
    // for (int i = 0 ; i < 2*game->numMines; i+=2){
    //     printf("%d %d\n",game->parplaymines[i],game->parplaymines[i+1]);
    // }
    // printf("playboard\n");
    // game->printBoard(game->playboard);


    

    return 0;
}