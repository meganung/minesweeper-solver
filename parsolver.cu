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

using namespace std;


__device__ float generate(curandState* globalState)
{
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int idy = blockIdx.y * blockDim.y + threadIdx.y;
    int ind = (idy * gridDim.x * blockDim.x) + idx;
    curandState localState = globalState[ind];
    float RANDOM = curand_uniform( &localState );
    globalState[ind] = localState;
    return RANDOM;
}

__device__ void chooseRandomMove(int* playboard, int* board, int height, int width, curandState* globalState, int* resx, int* resy) {
    *resx = int(generate(globalState) * height);
    *resy = int(generate(globalState) * width);
    // printf("in crm: %d %d\n",*resx, *resy);
    while (playboard[*resx * width + *resy] == 1) {
        *resx = (int)(generate(globalState) * height);
        *resy = (int)(generate(globalState) * width);
    }
}

//count uncovered adj mines (meaning already marked)
__device__ void countAdjMines(int* playboard, int* board, int height, int width, int x, int y, int* res) {
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
__device__  void revealNeighbors(int* playboard, int* board, int height, int width, int x, int y) {
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

__device__  void countUnrevealed(int* playboard, int* board, int height, int width, int x, int y, int* res) {
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
__device__  void markNeighbors(int* playboard, int* board, int height, int width, int* device_result, int* minesFound, int x, int y, int numMines) {
    for (int i = -1; i < 2; i++) {
        for (int j = -1; j < 2; j++) {
            int xi = x+i;
            int yi = y+j;
            if (xi >= 0 && xi < height && yi >= 0 && yi < width && !(i == 0 && j == 0)) {
                if (playboard[xi * width + yi] == 0) {
                    playboard[xi * width + yi] = 1;
                    int ogval = atomicAdd(minesFound, 1);
                    if (ogval < numMines) {
                        device_result[(ogval+1)*2] = xi;
                        device_result[(ogval+1)*2 + 1] = yi;
                    }
                }
            }
        }
    } 
    
}

__global__ void parSolveKernel(int* device_board, int* device_playboard, int* device_result, int* minesFound, int height, int width, int numMines, curandState* globalState) {
    int startheight = (blockIdx.y * blockDim.y + (threadIdx.y)) * CHUNK_DIM;
    int endheight = min(height, startheight + CHUNK_DIM);
    int startwidth = (blockIdx.x * blockDim.x + (threadIdx.x)) * CHUNK_DIM;
    int endwidth = min(width, startwidth + CHUNK_DIM);
    
    int guesses = 0;  
    while(*minesFound < numMines - 1) {
        int x, y;
        
        chooseRandomMove(device_playboard, device_board, height, width, globalState, &x, &y);
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
                        countAdjMines(device_playboard, device_board, height, width, i,j, &adjmines);
                        int unrevealed;
                        countUnrevealed(device_playboard, device_board, height, width, i,j, &unrevealed);
                        if (unrevealed != 0 ){
                            if (adjmines == device_board[i * width + j]) { //all mines found
                                //reveal neighbors
                                progress = true;
                                revealNeighbors(device_playboard, device_board, height, width, i,j);
                            }
                            if (unrevealed == device_board[i * width + j] - adjmines && unrevealed >= 0) {
                                progress = true;
                                markNeighbors(device_playboard, device_board, height, width, device_result, minesFound, i,j,numMines);
                            }
                        }
                        
                    }
                }
            }
        }
    }
}

__global__ void randomMoveKernel(int* device_board, int* device_playboard, int* device_result, int* minesFound, int height, int width, int numMines, curandState* globalState) {
    int x, y;
    chooseRandomMove(device_playboard, device_board, height, width, globalState, &x, &y);
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

__global__ void noGuessKernel(int* device_board, int* device_playboard, int* device_result, int* minesFound, int height, int width, int numMines, curandState* globalState) {
    int startheight = (blockIdx.y * blockDim.y + (threadIdx.y)) * CHUNK_DIM;
    int endheight = min(height, startheight + CHUNK_DIM);
    int startwidth = (blockIdx.x * blockDim.x + (threadIdx.x)) * CHUNK_DIM;
    int endwidth = min(width, startwidth + CHUNK_DIM);

    //closer waiting shared mem
    __shared__ int notdone;
    notdone = 1;
    
    //closer waiting
    while (notdone) {
        __syncthreads();
        if (threadIdx.x == 0 && threadIdx.y == 0) notdone = 0;
        __syncthreads();
        //end closer waiting
        bool progress = true;
        while (progress) {
            progress = false;
            for (int i = startheight; i < endheight; i++) {
                for (int j = startwidth; j < endwidth; j++) {
                    if (device_playboard[i * width + j] == 1 && device_board[i * width + j] != -1 ) { //clear square
                        int adjmines;
                        countAdjMines(device_playboard, device_board, height, width, i,j, &adjmines);
                        int unrevealed;
                        countUnrevealed(device_playboard, device_board, height, width, i,j, &unrevealed);
                        if (unrevealed != 0 ){
                            if (adjmines == device_board[i * width + j]) { //all mines found
                                //reveal neighbors

                                //closer waiting
                                if (progress == false) notdone = 1;

                                progress = true;
                                revealNeighbors(device_playboard, device_board, height, width, i,j);
                            }
                            if (unrevealed == device_board[i * width + j] - adjmines && unrevealed >= 0) {
                                //closer waiting
                                if (progress == false) notdone = 1;                             

                                progress = true;
                                markNeighbors(device_playboard, device_board, height, width, device_result, minesFound, i,j, numMines);
                            }
                        }
                        
                    }
                }
            }
        }  
        //closer waiting syncthreads
        __syncthreads();
    }
}

__global__ void setup_kernel( curandState* state, unsigned long seed )
{
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int idy = blockIdx.y * blockDim.y + threadIdx.y;
    int id = (idy * gridDim.x * blockDim.x) + idx;
    curand_init ( seed, id, 0, &state[id] );
}


double Game::parSolve(int iter) {

    // int totalBytes = sizeof(int) * height * width;

    // compute number of blocks and threads per block

    dim3 blockDim(BLOCK_DIM, BLOCK_DIM);
    dim3 gridDim((width + (blockDim.x * CHUNK_DIM) - 1) / (blockDim.x * CHUNK_DIM), (height + (blockDim.y * CHUNK_DIM) - 1) / (blockDim.y * CHUNK_DIM));
    // printf("%d %d\n",gridDim.x, gridDim.y);

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
    setup_kernel<<<gridDim, blockDim>>>(devStates,seed);

    // NO WAITING
    // parSolveKernel<<<gridDim, blockDim>>>(device_board, device_playboard, device_result, minesfound, height, width, numMines, devStates);

    /* START OF WAITING */
    int hostMinesFound = 0;
    while(hostMinesFound < numMines - 1) {     
        randomMoveKernel<<<1, 1>>>(device_board, device_playboard, device_result, minesfound, height, width, numMines, devStates);
        noGuessKernel<<<gridDim, blockDim>>>(device_board, device_playboard, device_result, minesfound, height, width, numMines, devStates);
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

void
printCudaInfo() {

    // for fun, just print out some stats on the machine

    int deviceCount = 0;
    cudaError_t err = cudaGetDeviceCount(&deviceCount);

    printf("---------------------------------------------------------\n");
    printf("Found %d CUDA devices\n", deviceCount);

    for (int i=0; i<deviceCount; i++) {
        cudaDeviceProp deviceProps;
        cudaGetDeviceProperties(&deviceProps, i);
        printf("Device %d: %s\n", i, deviceProps.name);
        printf("   SMs:        %d\n", deviceProps.multiProcessorCount);
        printf("   Global mem: %.0f MB\n",
               static_cast<float>(deviceProps.totalGlobalMem) / (1024 * 1024));
        printf("   CUDA Cap:   %d.%d\n", deviceProps.major, deviceProps.minor);
    }
    printf("---------------------------------------------------------\n");
}