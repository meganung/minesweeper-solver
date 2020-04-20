all: 
	  g++ -std=c++11 main.cpp game.cpp seqsolver.cpp -o minesweeper -g

par:
	  nvcc -std=c++11 parsolver.cu -O3 -m64 --gpu-architecture compute_61 -c -o objs/parsolve.o -g
	  g++ -std=c++11 -m64 -O3 -Wall -o parsolver main.cpp  game.cpp objs/parsolve.o seqsolver.cpp -L/usr/local/depot/cuda-10.2/lib64/ -lcudart -g

