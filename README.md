# Parallelized Minesweeper Solver
Final project for 15-418 at Carnegie Mellon University.

There are 3 different implementations besides the sequential version: CUDA, CUDA with shared memory, and OpenMP.
The CUDA implementation achieves the best speedup overall. 

## To run: 
``` 
make 
./minesweeper -h height -w width -n numMines -m 0,1,2,3 (seq, cuda, cuda w shared mem, openmp) -p (print) -t (testing) 
```

`-m` flag specifies the mode to run in. 0 means sequential, 1 means CUDA, 2 is CUDA with shared memory, and 3 is OpenMP. 

`-p` flag will print the game board and the results of mines found at the end. This flag should only be used on smaller boards.

`-t` flag runs tests. It will run multiple iterations of each implementation and report stats. Running the `-t` flag with a specific mode (with the `-m` flag) will run tests for just the specified mode. 