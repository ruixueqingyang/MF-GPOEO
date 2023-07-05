# MF-GPOEO

**The corresponding paper has been already in submission.**

./Src is the source code of our work.

./DEPO is our implementation of DEPO. The source code of **DEPO for GPU** was still **unavailable** online when our paper was submitted. (2023/06/16 13:12 UTC, https://projects.task.gda.pl/akrz/split)

./PerformanceMeasurement is our tool to measure GPU power, energy, SM frequency, memory frequency, execution time, etc.



## Compile and use our work

### Compiling
cd ./Src

Set the CUDA installation path in Makefile.

make -j8

Get the dynamic library (libGPOEO.so) of our work.

### Using
export CUDA_INJECTION64_PATH=<full_path>/libGPOEO.so # set environment variable of libGPOEO.so

<full_path>/GPU_Application.bin # run a GPU application (running for a period of time, such as 10 minutes)



## Compile and use DEPO (our implementation)

### Compiling
cd ./DEPO

Set the CUDA installation path in Makefile.

make -j8

Get the dynamic library (libDEPO.so) of DEPO.

### Using
export CUDA_INJECTION64_PATH=<full_path>/libGPOEO.so # set environment variable of libGPOEO.so

<full_path>/GPU_Application.bin # run a GPU application (running for a period of time, such as 10 minutes)



## Compile and use PerformanceMeasurement

### Compiling
cd ./PerformanceMeasurement

Set the CUDA installation path in Makefile.

make -j8

Get the measurement tool (PerfMeasure.bin).

### Using
The test.sh is an example.