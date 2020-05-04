#include <stdio.h>
#include <tuple>
#include <cuda.h>
#include <cuda_runtime.h>
#include <driver_functions.h>
#include "CycleTimer.h"
#include "game.h"
#include <curand.h>
#include <curand_kernel.h>

#define BLOCK_DIM 5
#define CHUNK_DIM 2
#define WIDTH 100
#define HEIGHT 100

using namespace std;


__device__ float s_generate(curandState* globalState)
{
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int idy = blockIdx.y * blockDim.y + threadIdx.y;
    int ind = (idy * gridDim.x * blockDim.x) + idx;
    curandState localState = globalState[ind];
    float RANDOM = curand_uniform( &localState );
    globalState[ind] = localState;
    return RANDOM;
}

__device__ void s_chooseRandomMove(int* playboard, int* board, int height, int width, curandState* globalState, int* resx, int* resy) {
    *resx = int(s_generate(globalState) * height);
    *resy = int(s_generate(globalState) * width);
    // printf("in crm: %d %d\n",*resx, *resy);
    while (playboard[*resx * width + *resy] == 1) {
        *resx = (int)(s_generate(globalState) * height);
        *resy = (int)(s_generate(globalState) * width);
    }
}

//count uncovered adj mines (meaning already marked)
__device__ void s_countAdjMines(int* playboard, int* board, int height, int width, int x, int y, int* res) {
    int c = 0;
    for (int i = -1; i < 2; i++) {
        for (int j = -1; j < 2; j++) {
            int xi = x+i;
            int yi = y+j;
            if (xi >= 0 && xi < height && yi >= 0 && yi < width && !(i == 0 && j == 0)) {
                if (playboard[xi * width + yi] == 1 && board[xi * width + yi] == -1) {
                    c++;
                } 
            }
        }
    } 
    *res = c;
}

//reveal neighbors not revealed yet
__device__  void s_revealNeighbors(int* playboard, int* board, int height, int width, int x, int y) {
    for (int i = -1; i < 2; i++) {
        for (int j = -1; j < 2; j++) {
            int xi = x+i;
            int yi = y+j;
            if (xi >= 0 && xi < height && yi >= 0 && yi < width && !(i == 0 && j == 0)) {
                if (playboard[xi * width + yi] == 0) {
                    playboard[xi * width + yi] = 1;
                    if (board[xi * width + yi] == -1) {
                        printf("OOPS: REVEALED BOMB BUT SHOULD HAVE BEEN OAK\n");
                    }
                } 
            }
        }
    } 
}

__device__  void s_countUnrevealed(int* playboard, int* board, int height, int width, int x, int y, int* res) {
    int c = 0;
    for (int i = -1; i < 2; i++) {
        for (int j = -1; j < 2; j++) {
            int xi = x+i;
            int yi = y+j;
            if (xi >= 0 && xi < height && yi >= 0 && yi < width && !(i == 0 && j == 0)) {
                if (playboard[xi * width + yi] == 0) {
                    c++;
                } 
            }
        }
    } 
    *res = c;
}
__device__  void s_markNeighbors(int* playboard, int* board, int height, int width, int* device_result, int* minesFound, int x, int y) {
    for (int i = -1; i < 2; i++) {
        for (int j = -1; j < 2; j++) {
            int xi = x+i;
            int yi = y+j;
            if (xi >= 0 && xi < height && yi >= 0 && yi < width && !(i == 0 && j == 0)) {
                if (playboard[xi * width + yi] == 0) {
                    playboard[xi * width + yi] = 1;
                    int ogval = atomicAdd(minesFound, 1);
                    device_result[(ogval+1)*2] = xi;
                    device_result[(ogval+1)*2 + 1] = yi;
                }
            }
        }
    } 
    
}

__global__ void s_parSolveKernel(int* device_board, int* device_playboard, int* device_result, int* minesFound, int height, int width, int numMines, curandState* globalState) {
    int startheight = (blockIdx.y * blockDim.y + (threadIdx.y)) * CHUNK_DIM;
    int endheight = min(height, startheight + CHUNK_DIM);
    int startwidth = (blockIdx.x * blockDim.x + (threadIdx.x)) * CHUNK_DIM;
    int endwidth = min(width, startwidth + CHUNK_DIM);

    int guesses = 0;  
    while(*minesFound < numMines - 1) {
        int x, y;
        s_chooseRandomMove(device_playboard, device_board, height, width, globalState, &x, &y);
        guesses++;
        if (device_board[x * width + y] == -1) {
            // printf("\n");
            // printf("oops %dth guess was a bomb big sad\n",guesses);
            *minesFound = numMines;
            return;
        } else {
            //reveal
            device_playboard[x * width + y] = 1;
        }
        bool progress = true;
        while (progress) {
            progress = false;
            for (int i = startheight; i < endheight; i++) {
                for (int j = startwidth; j < endwidth; j++) {
                    if (device_playboard[i * width + j] == 1 && device_board[i * width + j] != -1 ) { //clear square
                        int adjmines;
                        s_countAdjMines(device_playboard, device_board, height, width, i,j, &adjmines);
                        int unrevealed;
                        s_countUnrevealed(device_playboard, device_board, height, width, i,j, &unrevealed);
                        if (unrevealed != 0 ){
                            if (adjmines == device_board[i * width + j]) { //all mines found
                                //reveal neighbors
                                progress = true;
                                s_revealNeighbors(device_playboard, device_board, height, width, i,j);
                            }
                            if (unrevealed == device_board[i * width + j] - adjmines && unrevealed >= 0) {
                                progress = true;
                                s_markNeighbors(device_playboard, device_board, height, width, device_result, minesFound, i,j);
                            }
                        }
                        
                    }
                }
            }
        }
    }
}

__global__ void s_randomMoveKernel(int* device_board, int* device_playboard, int* device_result, int* minesFound, int height, int width, int numMines, curandState* globalState) {
    int x, y;
    s_chooseRandomMove(device_playboard, device_board, height, width, globalState, &x, &y);
    if (device_board[x * width + y] == -1) {
        // printf("\n");
        // printf("oops some guess was a bomb big sad\n");
        *minesFound = numMines;
        return;
    } else {
        //reveal
        device_playboard[x * width + y] = 1;
    }
}

__global__ void s_noGuessKernel(int* device_board, int* device_playboard, int* device_result, int* minesFound, int height, int width, int numMines, curandState* globalState) {
    int startheight = (blockIdx.y * blockDim.y + (threadIdx.y)) * CHUNK_DIM;
    int endheight = min(height, startheight + CHUNK_DIM);
    int startwidth = (blockIdx.x * blockDim.x + (threadIdx.x)) * CHUNK_DIM;
    int endwidth = min(width, startwidth + CHUNK_DIM);

    //init shared mem
    __shared__ int sharedPlayboard[WIDTH * HEIGHT];
    // __shared__ int sharedBoard[WIDTH * HEIGHT];
    __shared__ int notdone;


    for (int i = startheight; i < endheight; i++) {
        for (int j = startwidth; j < endwidth; j++) {
            if (i * width + j < 0 || i * width + j >= width * height) {
                printf("BAD 0\n");
            }
            sharedPlayboard[i * width + j] = device_playboard[i * width + j];
            // sharedBoard[i * width + j] = device_board[i * width + j];
        }
    }
    int prebsh = (((blockIdx.y * blockDim.y) * CHUNK_DIM) -1);
    int blockstartheight = max(0,prebsh);
    int blockendheight = min(height-1, (int)(blockstartheight + (CHUNK_DIM * blockDim.y) + 1));
    int blockstartwidth = max(0,(blockIdx.x * blockDim.x) * CHUNK_DIM );
    int blockendwidth = min(width, blockstartwidth + (CHUNK_DIM * blockDim.x) );
    int blockheight = blockendheight - blockstartheight;
    int blockwidth = blockendwidth - blockstartwidth;
    int copyamtheight = (blockheight + (blockDim.x * blockDim.y) - 1) / (blockDim.x * blockDim.y);
    int copyamtwidth = (blockwidth + (blockDim.x * blockDim.y) - 1) / (blockDim.x * blockDim.y);
    int threadid = threadIdx.y * blockDim.x + threadIdx.x;
    int threadstart, threadend;
    //top and bottom
    threadstart = blockstartwidth + (threadid * copyamtwidth);
    threadend = min(blockendwidth, (threadstart + copyamtwidth));
    for (int i = threadstart; i < threadend; i++) {
        if (blockstartheight * width + i < 0 || blockstartheight * width + i >= height * width || blockendheight * width + i < 0 || blockendheight * width + i >= height * width)  {
            printf("BAD 1 blockstartheight %d width %d i %d total: %d\n",blockendheight ,width , i,blockendheight * width + i);
        }
        sharedPlayboard[blockstartheight * width + i] = device_playboard[blockstartheight * width + i];
        // sharedBoard[blockstartheight * width + i] = device_board[blockstartheight * width + i];
        sharedPlayboard[blockendheight * width + i] = device_playboard[blockendheight * width + i];
        // sharedBoard[blockendheight * width + i] = device_board[blockendheight * width + i];
    }
    //left and right
    threadstart = blockstartheight + (threadid * copyamtheight);
    threadend = min(blockendheight, (threadstart + copyamtheight));
    for (int i = threadstart; i < threadend; i++) {
        if (i * width + blockstartwidth < 0 || i * width + blockendwidth < 0 || i * width + blockstartwidth >= height * width || i * width + blockendwidth >= height * width) {
            printf("BAD 2\n");
        }
        sharedPlayboard[i * width + blockstartwidth] = device_playboard[i * width + blockstartwidth];
        // sharedBoard[i * width + blockstartwidth] = device_board[i * width + blockstartwidth];
        sharedPlayboard[i * width + blockendwidth] = device_playboard[i * width + blockendwidth];
        // sharedBoard[i * width + blockstartwidth] = device_board[i * width + blockstartwidth];
    }
    __syncthreads();


    notdone = 1;
    while(notdone) {
        __syncthreads();
        if (threadIdx.x == 0 && threadIdx.y == 0) notdone = 0;
        __syncthreads();
        bool progress = true;
        while (progress) {
            progress = false;
            for (int i = startheight; i < endheight; i++) {
                for (int j = startwidth; j < endwidth; j++) {
                    if (sharedPlayboard[i * width + j] == 1 && device_board[i * width + j] != -1 ) { //clear square
                        int adjmines;
                        s_countAdjMines(sharedPlayboard, device_board, height, width, i,j, &adjmines);
                        int unrevealed;
                        s_countUnrevealed(sharedPlayboard, device_board, height, width, i,j, &unrevealed);
                        if (unrevealed != 0 ){
                            if (adjmines == device_board[i * width + j]) { //all mines found
                                //reveal neighbors

                                if (progress == false) notdone = 1;

                                progress = true;
                                s_revealNeighbors(sharedPlayboard, device_board, height, width, i,j);
                            }
                            if (unrevealed == device_board[i * width + j] - adjmines && unrevealed >= 0) {
                                if (progress == false) notdone = 1;

                                progress = true;
                                s_markNeighbors(sharedPlayboard, device_board, height, width, device_result, minesFound, i,j);
                            }
                        }
                        
                    }
                }
            }
        }
        __syncthreads();
    }
        //copy back
        for (int i = startheight; i < endheight; i++) {
            for (int j = startwidth; j < endwidth; j++) {
                if (sharedPlayboard[i * width + j] == 1) {
                    device_playboard[i * width + j] = sharedPlayboard[i * width + j];
                }
            }
        }
        //top and bottom
        threadstart = blockstartwidth + (threadid * copyamtwidth);
        threadend = min(blockendwidth, (threadstart + copyamtwidth));
        for (int i = threadstart; i < threadend; i++) {
            if (sharedPlayboard[blockstartheight * width + i] == 1) {
                device_playboard[blockstartheight * width + i] = sharedPlayboard[blockstartheight * width + i];
            }
            if (sharedPlayboard[blockendheight * width + i] == 1) {
                device_playboard[blockendheight * width + i] = sharedPlayboard[blockendheight * width + i];
            }
        }
        //left and right
        threadstart = blockstartheight + (threadid * copyamtheight);
        threadend = min(blockendheight, (threadstart + copyamtheight));
        for (int i = threadstart; i < threadend; i++) {
            if (sharedPlayboard[i * width + blockstartwidth] == 1) {
                device_playboard[i * width + blockstartwidth] = sharedPlayboard[i * width + blockstartwidth];
            }
            if (sharedPlayboard[i * width + blockstartwidth] == 1) {
                device_playboard[i * width + blockstartwidth] = sharedPlayboard[i * width + blockstartwidth];
            }
        }

    
}

__global__ void s_setup_kernel( curandState* state, unsigned long seed )
{
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int idy = blockIdx.y * blockDim.y + threadIdx.y;
    int id = (idy * gridDim.x * blockDim.x) + idx;
    curand_init ( seed, id, 0, &state[id] );
}


double Game::sharedParSolve(int iter) {

    // int totalBytes = sizeof(int) * height * width;

    // compute number of blocks and threads per block

    dim3 blockDim(BLOCK_DIM, BLOCK_DIM);
    dim3 gridDim((width + (blockDim.x * CHUNK_DIM) - 1) / (blockDim.x * CHUNK_DIM), (height + (blockDim.y * CHUNK_DIM) - 1) / (blockDim.y * CHUNK_DIM));
    // printf("gridDim: %d, %d\n",gridDim.x, gridDim.y);


    int* device_board;
    int* device_playboard;
    int* device_result;

    // allocate device memory buffers on the GPU using cudaMalloc
    int* minesfound;
    cudaMalloc(&minesfound,sizeof(int));
    cudaMalloc(&device_board,sizeof(int)*height*width);
    cudaMalloc(&device_playboard,sizeof(int)*height*width);
    cudaMalloc(&device_result,sizeof(int)*numMines*2);


    // start timing after allocation of device memory
    double startTime = CycleTimer::currentSeconds();

    // copy input arrays to the GPU using cudaMemcpy   
    cudaMemset(minesfound,-1,sizeof(int));
    cudaMemcpy(device_board,parboard,sizeof(int)*width*height,cudaMemcpyHostToDevice);
    cudaMemcpy(device_playboard,parplayboard,sizeof(int)*width*height,cudaMemcpyHostToDevice);
    cudaMemcpy(device_result,playmines,sizeof(int)*numMines*2,cudaMemcpyHostToDevice);

    double startTimeKernel = CycleTimer::currentSeconds();
    // run kernel

    //random
    curandState* devStates;
    cudaMalloc (&devStates, width * height * sizeof(curandState));
    srand(time(0) * iter);
    int seed = rand();
    s_setup_kernel<<<gridDim, blockDim>>>(devStates,seed);

    // NO WAITING
    // s_parSolveKernel<<<gridDim, blockDim>>>(device_board, device_playboard, device_result, minesfound, height, width, numMines, devStates);

    /* START OF WAITING */
    int hostMinesFound = 0;
    while(hostMinesFound < numMines - 1) {
        s_randomMoveKernel<<<1, 1>>>(device_board, device_playboard, device_result, minesfound, height, width, numMines, devStates);
        s_noGuessKernel<<<gridDim, blockDim>>>(device_board, device_playboard, device_result, minesfound, height, width, numMines, devStates);
        cudaMemcpy(&hostMinesFound,minesfound,sizeof(int),cudaMemcpyDeviceToHost);

    }
    // END OF WAITING

    cudaThreadSynchronize();
    double endTimeKernel = CycleTimer::currentSeconds(); 

    // copy result from GPU using cudaMemcpy
    cudaMemcpy(playmines,device_result,sizeof(int)*2*numMines,cudaMemcpyDeviceToHost);



    // end timing after result has been copied back into host memory
    double endTime = CycleTimer::currentSeconds();

    cudaError_t errCode = cudaPeekAtLastError();
    if (errCode != cudaSuccess) {
        fprintf(stderr, "WARNING: A CUDA error occured: code=%d, %s\n", errCode, cudaGetErrorString(errCode));
    }

    double overallDuration = endTime - startTime;
    // printf("Overall: %.3f ms\t\t[%.3f GB/s]\n", 1000.f * overallDuration, toBW(totalBytes, overallDuration));
    // double kernelDuration = endTimeKernel - startTimeKernel;
    // printf("Kernel: %.3f ms\t\t[%.3f GB/s]\n", 1000.f * kernelDuration, toBW(totalBytes, kernelDuration));

    // free memory buffers on the GPU
    cudaFree(device_board);
    cudaFree(device_playboard);
    cudaFree(device_result);
    cudaFree(minesfound);

    return overallDuration;
}
