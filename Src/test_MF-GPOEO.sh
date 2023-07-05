#!/bin/bash

# the environment variables
MFGPOEO_SO_PATH="<full_path>/Src/libGPOEO.so" # the path to libGPOEO.so
MFGPOEO_CONFIG_PATH="<full_path>/Src/RTX3080Ti.ini" # the path to the configuration file
MFGPOEO_OUTPUT_PATH="<full_path>/log.txt" # the path to the log file
MFGPOEO_WORK_MODE="PID" # the work mode

echo "export CUDA_INJECTION64_PATH=${MFGPOEO_SO_PATH}"
export CUDA_INJECTION64_PATH=${MFGPOEO_SO_PATH}
echo "export MFGPOEO_CONFIG_PATH=${MFGPOEO_CONFIG_PATH}"
export MFGPOEO_CONFIG_PATH
echo "export MFGPOEO_OUTPUT_PATH=${MFGPOEO_OUTPUT_PATH}"
export MFGPOEO_OUTPUT_PATH
echo "export MFGPOEO_WORK_MODE=${MFGPOEO_WORK_MODE}"
export MFGPOEO_WORK_MODE

# run a GPU application (running for a period of time, such as 10 minutes)
sudo <full_path>/GPU_Application.bin

echo "unset CUDA_INJECTION64_PATH"
unset CUDA_INJECTION64_PATH
echo "unset MFGPOEO_CONFIG_PATH"
unset MFGPOEO_CONFIG_PATH
echo "unset MFGPOEO_OUTPUT_PATH"
unset MFGPOEO_OUTPUT_PATH
echo "unset MFGPOEO_WORK_MODE"
unset MFGPOEO_WORK_MODE
