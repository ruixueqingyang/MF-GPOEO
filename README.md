# MF-GPOEO

**The corresponding paper has been already in submission.**

`./Src` is the source code of our work.

`./DEPO` is our implementation of DEPO for GPU. The source code of **DEPO for GPU** was still **unavailable** online when our paper was submitted. (2023/06/16 13:12 UTC, https://projects.task.gda.pl/akrz/split)

`./PerformanceMeasurement` is our tool to measure GPU power, GPU energy, GPU SM frequency, GPU memory frequency, execution time, CPU power, CPU energy, etc.



## Compile and use our work

### Compiling
`cd ./Src`

Set the CUDA installation path (`CUDA_DIR`) in `Makefile`.

`make -j8`

Get the dynamic library (`libGPOEO.so`) of our work.

### Using
```shell
# set environment variables
export CUDA_INJECTION64_PATH=<full_path>/libGPOEO.so # the path to libGPOEO.so
export MFGPOEO_CONFIG_PATH=<full_path>/Src/RTX3080Ti.ini # the path to the configuration file
export MFGPOEO_OUTPUT_PATH=<full_path>/log.txt # the path to the log file
export MFGPOEO_WORK_MODE=PID # the work mode

# run a GPU application (running for a period of time, such as 10 minutes)
sudo <full_path>/GPU_Application.bin
```
The `test_MF-GPOEO.sh` is an example.

## Compile and use DEPO (our implementation)

### Compiling
`cd ./DEPO`

Set the CUDA installation path (`CUDA_DIR`) in `Makefile`.

`make -j8`

Get the dynamic library (`libDEPO.so`) of DEPO.

### Using
```shell
# set environment variables
export CUDA_INJECTION64_PATH=<full_path>/libDEPO.so # the path to libDEPO.so
export DEPO_CONFIG_PATH=<full_path>/DEPO/RTX3080Ti.ini # the path to the configuration file
export DEPO_OUTPUT_PATH=<full_path>/log.txt # the path to the log file
export DEPO_WORK_MODE=DEPO # the work mode

# run a GPU application (running for a period of time, such as 10 minutes)
sudo <full_path>/GPU_Application.bin
```
The `test_DEPO.sh` is an example.


## Compile and use PerformanceMeasurement

### Compiling
`cd ./PerformanceMeasurement`

Set the CUDA installation path (`CUDA_DIR`) in `Makefile`.

`make -j8`

Get the measurement tool (`PerfMeasure.bin`).

### Using
The `test.sh` is an example.
```shell
sudo test.sh
```