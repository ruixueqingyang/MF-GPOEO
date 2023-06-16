/*******************************************************************************
Copyright(C), 2022-2022, 瑞雪轻飏
     FileName: measure.cpp
       Author: 瑞雪轻飏
      Version: 0.01
Creation Date: 20221024
  Description: CUPTI/NVML测量/调节相关函数和类
       Others: 
*******************************************************************************/
#ifndef __MEASURE_H
#define __MEASURE_H

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <iomanip>
#include <iostream>
#include <vector>
#include <system_error>
#include <string.h>
#include <vector>
#include <assert.h>
#include <math.h>
#include <fstream>
#include <sstream>
#include <math.h>
#include <map>
#include <unordered_map>
#include <iterator>
#include <algorithm>

#include <pthread.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <semaphore.h>

#include <nvml.h>
#include <cupti.h>
#include <cuda.h>
#include <cuda_runtime.h>

#include "CUPTIBuffer.h"
#include "kernel_data.h"
// #include "global_var.h"

// NVML 采样间隔 (ms)
#define SAMPLE_INTERVAL 100
// 记录历史数据的 vector 的空间
#define VECTOR_MEASURE_DATA_SIZE 16
#define VECTOR_MEASURE_BUFFER_SIZE 32

// CUPTI 单次测量的 buffer 大小
// 32 MB
// #define CUPTI_BUF_SIZE (4 * 1024 * 1024)
// #define CUPTI_BUF_SIZE (8 * 1024 * 1024)
#define CUPTI_BUF_SIZE (16 * 1024 * 1024)
// #define CUPTI_BUF_SIZE (32 * 1024 * 1024)
// #define CUPTI_BUF_SIZE (128 * 1024 * 1024)

void CUPTIAPI bufferRequested(uint8_t **buffer, size_t *size, size_t *maxNumRecords);
void CUPTIAPI bufferCompleted(CUcontext ctx, uint32_t streamId, uint8_t *buffer, size_t size, size_t validSize);
void AlarmSampler(int signum);

// 传递给数据分析/处理线程的数据的数据结构
struct THREAD_ANALYSIS_ARGS {
public:
    int index;
    double TimeStamp;
    float SampleInterval;
    float MeasureDuration;
    unsigned int SMClkUpperLimit;
    unsigned int MemClkUpperLimit;
    bool cupti_flag;
};

// nvml 配置和初始化
class NVML_CONFIG {
public:
    int DeviceID;
    nvmlDevice_t nvmlDevice;
    // float SampleInterval; // (s) Sampling Interval

    void SetPowerLimit(int PowerLimit);
    void ResetPowerLimit();
    void SetSMClkRange(unsigned int LowerLimit, unsigned int UpperLimit);
    void SetSMClkUpperLimit(unsigned int SMClkLimit);
    void ResetSMClkLimit();
    void SetMemClkRange(unsigned int LowerLimit, unsigned int UpperLimit);
    void SetMemClkUpperLimit(unsigned int MemClkLimit);
    void ResetMemClkLimit();

    void init(int id) {
        DeviceID = id;
        // SampleInterval = interval;
        nvmlReturn_t nvmlResult;
        nvmlResult = nvmlInit();
        if (NVML_SUCCESS != nvmlResult) {
            std::cout << "EPOPT_NVML::Init: Failed to init NVML (NVML ERROR INFO: " << nvmlErrorString(nvmlResult) << ")." << std::endl;
            exit(-1);
        }
        nvmlResult = nvmlDeviceGetHandleByIndex(DeviceID, &nvmlDevice);
        if (NVML_SUCCESS != nvmlResult)
        {
            std::cout << "NVML_CONFIG::init: Failed to get handle for device " << DeviceID << " (NVML ERROR INFO: " << nvmlErrorString(nvmlResult) << ")." << std::endl;
            exit(-1);
        }
    }

    NVML_CONFIG(int id) {
        init(id);
    }

    NVML_CONFIG() {
        DeviceID = 0;
    }

    void uninit() {
        nvmlReturn_t nvmlResult = nvmlShutdown();
        if (NVML_SUCCESS != nvmlResult)
        {
            std::cout << "NVML_CONFIG::uninit: Failed to nvmlShutdown NVML (NVML ERROR INFO: " << nvmlErrorString(nvmlResult) << ")." << std::endl;
            exit(-1);
        }
    }
};

void printSMClkUpperLimit(unsigned int SMClkUpperLimit);
void printMemClkUpperLimit(unsigned int MemClk);
void printTimeStamp();

#endif