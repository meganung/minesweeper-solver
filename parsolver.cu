#include <stdio.h>
#include <tuple>
#include <cuda.h>
#include <cuda_runtime.h>
#include <driver_functions.h>
#include "CycleTimer.h"
#include "game.h"
#include <curand.h>
#include <curand_kernel.h>


using namespace std;

float toBW(int bytes, float sec) {
    return static_cast<float>(bytes) / (1024. * 1024. * 1024.) / sec;
  }

__device__ float generate(curandState* globalState, int ind)
{
    //int ind = threadIdx.x;
    curandState localState = globalState[ind];
    float RANDOM = curand_uniform( &localState );
    globalState[ind] = localState;
    return RANDOM;
}

__device__ void chooseRandomMove(int* playboard, int* board, int height, int width, curandState* globalState, int* resx, int* resy) {
    printf("IN CHOOSE RM\n");
    *resx = int(generate(globalState, 0) * height);
    *resy = int(generate(globalState, 0) * width);
    printf("in crm: %d %d\n",*resx, *resy);
    while (playboard[*resx * width + *resy] == 1) {
        *resx = (int)(generate(globalState, 0) * height);
        *resy = (int)(generate(globalState, 0) * width);
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
                        printf("DID A BAD\n");
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
__device__  void markNeighbors(int* playboard, int* board, int height, int width, int* device_result, int* minesFound, int x, int y) {
    for (int i = -1; i < 2; i++) {
        for (int j = -1; j < 2; j++) {
            int xi = x+i;
            int yi = y+j;
            if (xi >= 0 && xi < height && yi >= 0 && yi < width && !(i == 0 && j == 0)) {
                if (playboard[xi * width + yi] == 0) {
                    playboard[xi * width + yi] = 1;
                    //TODO: make atomic
                    device_result[*minesFound*2] = xi;
                    device_result[*minesFound*2 + 1] = yi;
                    (*minesFound)++;
                }
            }
        }
    } 
    
}

__global__ void parSolveKernel(int* device_board, int* device_playboard, int* device_result, int* minesFound, int height, int width, int numMines, curandState* globalState) {
    printf("IN PARSOLVEKERNEL\n");
    int guesses = 0;  
    while(*minesFound < numMines) {
        int x, y;
        printf("IN THE WHOLE LOO\n");
        chooseRandomMove(device_playboard, device_board, height, width, globalState, &x, &y);
        guesses++;
        printf("%d %d\n",x,y);
        if (device_board[x * width + y] == -1) {
            printf("\n");
            printf("oops %dth guess was a bomb big sad\n",guesses);
            return;
        } else {
            //reveal
            device_playboard[x * width + y] = 1;
        }
        bool progress = true;
        while (progress) {
            progress = false;
            for (int i = 0; i < height; i++) {
                for (int j = 0; j < width; j++) {
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
                                markNeighbors(device_playboard, device_board, height, width, device_result, minesFound, i,j);
                            }
                        }
                        
                    }
                }
            }
        }
    }
}

__global__ void setup_kernel( curandState * state, unsigned long seed )
{
    int id = threadIdx.x;
    curand_init ( seed, id, 0, &state[id] );
}


void Game::parSolve() {

    int totalBytes = sizeof(int) * height * width;

    // compute number of blocks and threads per block
    // const int threadsPerBlock = 512;
    // const int blocks = (N + threadsPerBlock - 1) / threadsPerBlock;
    const int threadsPerBlock = 1;
    const int blocks = 1;
    // int N = width * height;

    int* device_board;
    int* device_playboard;
    int* device_result;

    //
    // TODO allocate device memory buffers on the GPU using cudaMalloc
    //

    printf("01\n");
    int* minesfound;
    cudaMalloc(&minesfound,sizeof(int));
    cudaMalloc(&device_board,sizeof(int)*height*width);
    cudaMalloc(&device_playboard,sizeof(int)*height*width);
    cudaMalloc(&device_result,sizeof(int)*numMines*2);
    printf("02\n");




    // start timing after allocation of device memory
    double startTime = CycleTimer::currentSeconds();

    //
    // TODO copy input arrays to the GPU using cudaMemcpy
    //
    

    cudaMemcpy(device_board,parboard,sizeof(int)*width*height,cudaMemcpyHostToDevice);
    cudaMemcpy(device_playboard,parplayboard,sizeof(int)*width*height,cudaMemcpyHostToDevice);
    cudaMemcpy(device_result,parplaymines,sizeof(int)*numMines*2,cudaMemcpyHostToDevice);

    printf("04\n");

    double startTimeKernel = CycleTimer::currentSeconds();
    // run kernel

    //random
    curandState* devStates;
    printf("1\n");
    cudaMalloc (&devStates, width * height * sizeof(curandState));
    srand(time(0));
    int seed = rand();
    printf("2\n");
    setup_kernel<<<blocks, threadsPerBlock>>>(devStates,seed);
    printf("3\n");
    parSolveKernel<<<blocks, threadsPerBlock>>>(device_board, device_playboard, device_result, minesfound, height, width, numMines, devStates);
    printf("4\n");

    cudaThreadSynchronize();
    double endTimeKernel = CycleTimer::currentSeconds(); 

    //
    // TODO copy result from GPU using cudaMemcpy
    //


    cudaMemcpy(parplaymines,device_result,sizeof(int)*2*numMines,cudaMemcpyDeviceToHost);



    // end timing after result has been copied back into host memory
    double endTime = CycleTimer::currentSeconds();

    cudaError_t errCode = cudaPeekAtLastError();
    if (errCode != cudaSuccess) {
        fprintf(stderr, "WARNING: A CUDA error occured: code=%d, %s\n", errCode, cudaGetErrorString(errCode));
    }

    double overallDuration = endTime - startTime;
    printf("Overall: %.3f ms\t\t[%.3f GB/s]\n", 1000.f * overallDuration, toBW(totalBytes, overallDuration));
    double kernelDuration = endTimeKernel - startTimeKernel;
    printf("Kernel: %.3f ms\t\t[%.3f GB/s]\n", 1000.f * kernelDuration, toBW(totalBytes, kernelDuration));

    // TODO free memory buffers on the GPU
    cudaFree(device_board);
    cudaFree(device_playboard);
    cudaFree(device_result);

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