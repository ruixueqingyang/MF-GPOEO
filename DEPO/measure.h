/*******************************************************************************
Copyright(C), 2022-2023, 瑞雪轻飏
     FileName: measure.cpp
       Author: 瑞雪轻飏
      Version: 0.01
Creation Date: 20230413
  Description: CUPTI/NVML测量/调节相关函数和类
       Others: 
*******************************************************************************/
#ifndef __MEASURE_H
#define __MEASURE_H

#include "includes.h"

#include <nvml.h>
#include <cupti.h>
#include <cuda.h>
#include <cuda_runtime.h>

// #include "CUPTIBuffer.h"
// #include "kernel_data.h"

class KERNEL_DATA
{
public:
    int NumKnls;
    // float KnlPerSec;
    void init(){
        NumKnls = 0;
        // KnlPerSec = 0.0;
    }
    KERNEL_DATA(){
        init();
    }
};

class RAW_DATA
{
public:

    float Eng, T, KnlPerSec;
    int NumKnls;
    int BufLen, indexIdle;
    vector<KERNEL_DATA> vecData;
    deque<mutex> queLock;

    double IdleTime;
    unsigned int long long nvml_measure_count;
    double avgPwr, sumPwr; // J; 测量时长内平均功率, 加和功率
    double avgSMClk, avgMemClk; // MHz; 平均核心频率, 平均显存频率
    double avgGPUUtil, avgMemUtil; // %; 平均核心占用率, 平均显存IO占用率
    double maxPwr, maxSMClk, maxMemClk, maxGPUUtil, maxMemUtil; // 各个数据的最大值
    double sumSMClk, sumMemClk; // MHz
    double sumGPUUtil, sumMemUtil; // %

    void lock() {
        for (auto& Lock : queLock)
        {
            Lock.lock();
        }
    }

    void unlock() {
        for (auto& Lock : queLock)
        {
            Lock.unlock();
        }
    }

    void init() {
        T = 0.0;
        NumKnls = 0;
        KnlPerSec = 0.0;
        indexIdle = 0;
        IdleTime = 0.0;
        nvml_measure_count = 0;

        avgPwr = 0.0; maxPwr = 0.0; sumPwr = 0.0; Eng = 0.0;
        avgSMClk = 0.0; maxSMClk = 0.0; sumSMClk = 0.0;
        avgMemClk = 0.0; maxMemClk = 0.0; sumMemClk = 0.0;
        avgMemUtil = 0.0; maxMemUtil = 0.0; sumMemUtil = 0.0;
        avgGPUUtil = 0.0; maxGPUUtil = 0.0; sumGPUUtil = 0.0;
    }

    void init(int inBufLen) {
        init();
        BufLen = inBufLen;
        vecData.resize(BufLen);
        queLock.resize(BufLen);
        unlock();
    }

    void reset() {
        lock();
        init();
        indexIdle = 0;
        for (auto& data : vecData)
        {
            data.init();
        }
        unlock();
    }

    RAW_DATA() {
        init();
        BufLen = 0;
        vecData.clear();
        queLock.clear();
    }
};

class MEASURED_DATA
{
public:

    float Eng, T, avgPwr, KnlPerSec;
    int NumKnls;
    int Gear, SMGear, MemGear, PwrGear;
    float Obj, PerfLoss;
    bool flagObj;

    void init() {
        Eng = 0.0;
        T = 0.0;
        avgPwr = 0.0;
        NumKnls = 0;
        KnlPerSec = 0.0;
        Gear = 0;
        SMGear = 0;
        MemGear = 0;
        PwrGear = 0;
        Obj = 0.0;
        PerfLoss = 0.0;
        flagObj = false;
    }

    void clear() {
        init();
    }

    MEASURED_DATA() {
        init();
    }
};

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