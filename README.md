# GreenWaves Technologies software test

## Technical levels

Depending on the position you applied for you will need to follow different
instructions.

The levels are:

1. intern
2. new graduate
3. experienced

You need to follow instructions of up to your level.
For example, a new graduate needs to follow "intern" and "new graduate"
instructions.

## Instructions

This code should run on GAP8 GVSoC.

1. (intern) Write a program which would make all the cores print
   “hello world [cluster_id, core_id]”.
2. (intern) Write a bash script named `launch.sh` that compiles and launches
   your code.
3. (intern) Write a program which copy two square matrices (minimum size 64x64)
   of type `unsigned short` from L2 memory to L1 memory.
   Then produce the results of the two following operations by using the
   cluster and send the result matrices from the L1 to L2:
    - Addition of the 2 matrices
    - Multiplication of the 2 matrices
4. (new graduate) Extend the previous `launch.sh` script to accept a matrix
   size parameter.
   For example, entering `./launch.sh -s 128` will select a matrix size of
   128x128. Use a default value of 64 when size is not specified.
5. (new graduate) Using the result of the matrices multiplication above, do a
   convolution of this matrix with the filter below:
   `unsigned short filter[9] = {-1, -2, -1, 0, 0, 0, 1, 2, 1};`
6. (experienced) Optimize all previous operations as efficiently as possible.
