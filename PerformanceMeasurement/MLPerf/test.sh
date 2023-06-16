#!/bin/bash

# export CUDA_VISIBLE_DEVICES=1

PM="./PerfMeasure.bin"
PROF_DATA_DIR="/home/user/PerformanceMeasurement"

# GPU index, 功率/频率配置, 运行次数
GPUName="RTX2080Ti"
GPUIndex="1"
TuneType="SM_RANGE"

SampleInterval="100"
PowerThreshold="1.65"
PMFlagBase="-e -i "${GPUIndex}" -s "${SampleInterval}" -t "${PowerThreshold}" -m DAEMON -trace"
# -e -i 1 -s 100 -t 1.65 -m DAEMON

SleepInterval="15"

source "/home/user/PerformanceMeasurement/Msg2EPRT.sh"

# 启动运行时
echo "ExitMeasurement"
ExitMeasurement

echo "sleep 3s"
sleep 3s

echo "${PM} ${PMFlagBase} &"
${PM} ${PMFlagBase} &

echo "sleep 3s"
sleep 3s

PM_OUT=${PROF_DATA_DIR}/"Prof.txt"
echo "ResetMeasurement ${PM_OUT}"
ResetMeasurement ${PM_OUT}
echo "sleep 2s"
sleep 2s
echo "StartMeasurement"
StartMeasurement

echo "sleep 15s"
sleep 15s

# # execute scripts
# for Script in ${SCRIPTS_LIST[@]};
# do
#     source ${Script}
# done

echo "StopMeasurement"
StopMeasurement

echo "sleep 3s"
sleep 3s

echo "ExitMeasurement"
ExitMeasurement

echo "sleep 2s"
sleep 2s
echo "测试完毕"