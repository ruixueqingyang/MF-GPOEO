#!/bin/bash

# Make sure the IP address (127.0.0.1:7777) is available.

PM="<full_path>/PerfMeasure.bin"

GPUIndex="1" # the number of the GPU to be measured
SampleInterval="100" # sample interval (ms)
PowerThreshold="30" # static power of GPU (W), excluding static power consumption

# set PerfMeasure as daemon
PMFlagBase="-e -i "${GPUIndex}" -s "${SampleInterval}" -t "${PowerThreshold}" -m DAEMON -trace"

# You can control the PerfMeasure deamon through UDP messages. The available UDP messages are implemented with three languages: Msg2EPRT.sh Msg2EPRT.py Msg2EPRT.cpp
source "./Msg2EPRT.sh" 

# exit an existing PerfMeasure daemon
echo "ExitMeasurement"
ExitMeasurement
echo "sleep 3s"
sleep 3s

# start a new process of PerfMeasure as daemon
echo "sudo ${PM} ${PMFlagBase} &"
sudo ${PM} ${PMFlagBase} &
echo "sleep 1s"
sleep 1s




# You can repeat line 34~54 to measure multiple applications

# send the output path of the measurement result file to the PerfMeasure daemon and reset measurement
PM_OUT="<full_path>/Prof.txt"
echo "ResetMeasurement ${PM_OUT}"
ResetMeasurement ${PM_OUT}
sleep 1s

# start measurement
echo "StartMeasurement"
StartMeasurement

TimeStamp "BEGIN" # send a time stamp request and its tag to the PerfMeasure daemon
# run your application
/<full_path>/your_application.bin
TimeStamp "END" # send a time stamp request and its tag to the PerfMeasure daemon

# stop measurement
echo "StopMeasurement"
StopMeasurement

echo "sleep 2s"
sleep 2s




# exit the existing PerfMeasure daemon
echo "ExitMeasurement"
ExitMeasurement

echo "sleep 2s"
sleep 2s
