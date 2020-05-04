all: 	  
	  nvcc -std=c++11 parsolver.cu -O3 -m64 --gpu-architecture compute_61 -c -o objs/parsolve.o -g
	  nvcc -std=c++11 sharedparsolver.cu -O3 -m64 --gpu-architecture compute_61 -c -o objs/sharedparsolve.o -g
	  g++ -std=c++11 -m64 -O3 -Wall -o minesweeper main.cpp  game.cpp objs/parsolve.o objs/sharedparsolve.o ompsolver.cpp seqsolver.cpp -L/usr/local/depot/cuda-10.2/lib64/ -lcudart -g -fopenmp -DOMP

