#!bin/bash

if [ "$1" = "hello" ]; then
    export PROGRAM=test_greenwave_helloworld.c
elif [ "$1" = "matrix" ]; then
    export PROGRAM=test_greenwave_matrix.c
fi

make clean all run PMSIS_OS=freertos platform=gvsoc