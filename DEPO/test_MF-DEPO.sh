#!/bin/bash

# the environment variables
DEPO_SO_PATH="<full_path>/DEPO/libDEPO.so" # the path to libDEPO.so
DEPO_CONFIG_PATH="<full_path>/DEPO/RTX3080Ti.ini" # the path to the configuration file
DEPO_OUTPUT_PATH="<full_path>/log.txt" # the path to the log file
DEPO_WORK_MODE="DEPO" # the work mode

echo "export CUDA_INJECTION64_PATH=${DEPO_SO_PATH}"
export CUDA_INJECTION64_PATH=${DEPO_SO_PATH}
echo "export DEPO_CONFIG_PATH=${DEPO_CONFIG_PATH}"
export DEPO_CONFIG_PATH
echo "export DEPO_OUTPUT_PATH=${DEPO_OUTPUT_PATH}"
export DEPO_OUTPUT_PATH
echo "export DEPO_WORK_MODE=${DEPO_WORK_MODE}"
export DEPO_WORK_MODE

# run a GPU application (running for a period of time, such as 10 minutes)
sudo <full_path>/GPU_Application.bin

echo "unset CUDA_INJECTION64_PATH"
unset CUDA_INJECTION64_PATH
echo "unset DEPO_CONFIG_PATH"
unset DEPO_CONFIG_PATH
echo "unset DEPO_OUTPUT_PATH"
unset DEPO_OUTPUT_PATH
echo "unset DEPO_WORK_MODE"
unset DEPO_WORK_MODE
