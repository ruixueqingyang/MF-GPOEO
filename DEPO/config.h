/*******************************************************************************
Copyright(C), 2022-2023, 瑞雪轻飏
     FileName: config.h
       Author: 瑞雪轻飏
      Version: 0.01
Creation Date: 20230413
  Description: 全局配置定义
       Others: 
*******************************************************************************/

#ifndef __CONFIG_H
#define __CONFIG_H

#include "includes.h"

#include <nvml.h>
#include <cupti.h>
#include <cuda.h>
#include <cuda_runtime.h>

enum WORK_MODE{DEPO, MEASURE, STATIC, EVALUATE};

enum OPTIMIZATION_OBJECTIVE
{
    ENERGY, EDP, ED2P, EDS
};

class DEPO_CONFIG{
public:
    // 配置文件路径 和 优化过程记录输出路径
    std::string ConfigPath, OutPath;

    WORK_MODE WorkMode;
    OPTIMIZATION_OBJECTIVE OptObj;
    float PerfLossConstraint; // 性能损失约束

    std::string GPUName;
    int GPUID;
    double DefaultPowerLimit; // 默认功率上限 (W)
    unsigned int MaxSMClk, MaxMemClk; // 通过 nvidia-smi 查询到的最大频率
    int NumSMs; // SM 数量
    int CoresPerSM; // 一个 SM 中 CUDA Core 数量
    int NumCores; // CUDA Core 总量
    float MeasureWindowDuration; // 测量区间时长 单位 秒s
    float SampleInterval; // 采样时间间隔 单位 秒s

    double StartTimeStamp; // 开始的绝对时间戳
    volatile uint32_t initialized;
    CUpti_SubscriberHandle  subscriber;
    volatile uint32_t detachCupti;
    // int frequency;
    int KernelCountEnabled;
    int tracingEnabled;
    int isMeasuring; // 是否在测量 包括 NVML 和 CUPTI, 任意都算是在测量
    int terminateThread;
    int istimeout;
    pthread_t dynamicThread;
    pthread_mutex_t mutexFinalize;
    pthread_cond_t mutexCondition;

    void init() {
        WorkMode = WORK_MODE::DEPO;
        OptObj = OPTIMIZATION_OBJECTIVE::EDS;
        GPUID = 1;
        StartTimeStamp = 0.0; // 开始的绝对时间戳
        MeasureWindowDuration = 6.4;
        SampleInterval = 0.1;
        initialized = 0;
        subscriber = 0;
        detachCupti = 0;
        KernelCountEnabled = 0;
        tracingEnabled = 0;
        isMeasuring = 0;
        terminateThread = 0;
        istimeout = 0;
        mutexFinalize = PTHREAD_MUTEX_INITIALIZER;
        mutexCondition = PTHREAD_COND_INITIALIZER;
    }

    DEPO_CONFIG(){
        init();
    }
};

#endif