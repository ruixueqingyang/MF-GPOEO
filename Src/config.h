/*******************************************************************************
Copyright(C), 2022-2022, 瑞雪轻飏
     FileName: CUPTIBuffer.h
       Author: 瑞雪轻飏
      Version: 0.01
Creation Date: 20221014
  Description: 1. 定义 CUPTI 测量时 返回给用户的 缓冲区
       Others: //其他内容说明
*******************************************************************************/

#ifndef __CONFIG_H
#define __CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <iomanip>
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
#include <atomic>

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

enum WORK_MODE{PID, KERNEL, STATIC, PREDICT};

enum OPTIMIZATION_OBJECTIVE
{
    ENERGY, EDP, ED2P
};

// Global Structure
class GLOBAL_CONFIG{
public:
    // 配置文件路径 和 优化过程记录输出路径
    std::string ConfigPath, OutPath;

    // GPU 硬件参数
    std::string GPUName;
    int gpu_id; // GPU 编号
    int CUDADriverVersion; // CUDA 驱动版本
    double DefaultPowerLimit; // 默认功率上限 (W)
    unsigned int MaxSMClk, MaxMemClk; // 通过 nvidia-smi 查询到的最大频率
    float avgSMClkStep;
    int NumSMs; // SM 数量
    int CoresPerSM; // 一个 SM 中 CUDA Core 数量
    int NumCores; // CUDA Core 总量

    // 性能损失约束
    double objective; // 控制目标
    double ObjRelax; // 适当减小性能损失约束, 即留一些余量, 减小违反原始约束的可能
    WORK_MODE WorkMode;
    OPTIMIZATION_OBJECTIVE OptObj;

    // PID 的三个系数
    double kp, ki, kd; // PID 参数: 比例, 积分, 微分 系数

    // 测量/数据库更新/ExeGap时间匹配/复合性能指标计算 算法相关参数
    float ExeTimeRadius; //  聚类时判断 执行时间 的半径 (百分比)
    float GapTimeRadius; //  聚类时判断 Gap时间 的半径 (百分比)
    float StrangePerfThreshold; // 异常性能指标阈值, 限制过小的性能损失

    float MeasureWindowDuration, WindowDurationMin, WindowDurationMax; // 测量窗口长度 及 上下限 (s)
    float Pct_Inc; // < Pct_Inc 时 尝试增大窗口长度
    float Pct_Ctrl; // > Pct_Ctrl 时 进行 PID 控制, 更新最优能耗配置
    float discount; // 融合 Base 历史数据 时的 衰减系数
    float GapDiscount; // 计算性能指标 且 Gap部分匹配不好时 Gap 的衰减系数
    float GapDiscountOrigin; // 计算性能指标 且 Gap部分匹配不好时 Gap 的衰减系数
    int BaseUpdateCycle; // 基准更新频次
    int NumOptConfig; // 确定稳定所需的最近最优配置数量
    int StableSMClkStepRange; // 判断 PID 控制稳定 所用到的 阈值参数, 表示 频率档位的数量

    int PhaseWindow; // 阶段检测 所用的 窗口/数据数量
    float PhaseChangeThreshold; // 监测阶段变化 以 重启优化 所用到的 阈值参数, 表示 低开销执行向量 的变化百分比

    double StartTimeStamp; // 开始的绝对时间戳
    // 测量启停控制相关全局变量
    volatile uint32_t initialized;
    CUpti_SubscriberHandle  subscriber;
    volatile uint32_t detachCupti;
    int tracingEnabled; // CUPTI 是否在测量
    int isMeasuring; // 是否在测量 包括 NVML 和 CUPTI, 任意都算是在测量
    int terminateThread;
    int istimeout;
    // int RefCountAnalysis;
    std::atomic<int> RefCountAnalysis;
    pthread_t dynamicThread;
    pthread_mutex_t mutexFinalize;
    pthread_cond_t mutexCondition;


    void init() {

        GPUName = "NVIDIA GeForce RTX 3080 Ti";
        gpu_id = 1; // GPU 编号
        DefaultPowerLimit = 350; // 默认功率上限 350W
        MaxSMClk = 2160;
        MaxMemClk = 9501;
        NumSMs = 80; // SM 数量
        CoresPerSM = 128; // 一个 SM 中 CUDA Core 数量
        NumCores = 10240; // CUDA Core 总量
        
        objective = (double)5 / 100; // 控制目标: 性能损失阈值
        ObjRelax = 0.005; // 适当减小性能损失约束, 即留一些余量, 减小违反原始约束的可能
        objective -= ObjRelax;
        WorkMode = WORK_MODE::PID;
        OptObj = OPTIMIZATION_OBJECTIVE::ENERGY;

        kp = 2.8; // PID 系数
        ki = 0.002;
        kd = 0.001;

        ExeTimeRadius = 0.12; // 聚类时判断 执行时间 的半径
        GapTimeRadius = 0.10; // 聚类时判断 Gap时间 的半径
        StrangePerfThreshold = 0.1; // 异常性能指标阈值, 限制过小的性能损失

        MeasureWindowDuration = 2.0;
        WindowDurationMin = 1.0;
        WindowDurationMax = 20.0;
        Pct_Inc = 0.7; // < Pct_Inc 时 尝试增大窗口长度
        Pct_Ctrl = 0.6; // > Pct_Ctrl 时 进行 PID 控制, 更新最优能耗配置
        discount = 0.7; // 融合 Base 历史数据 时的 衰减系数
        GapDiscount = 0.65; // 计算性能指标 且 Gap部分匹配不好时 Gap 的衰减系数
        GapDiscountOrigin = 0.65; // 计算性能指标 且 Gap部分匹配不好时 Gap 的衰减系数
        BaseUpdateCycle = 5; // 基准更新频次
        NumOptConfig = 3; // 确定稳定所需的最近最优配置数量
        StableSMClkStepRange = 8; // 判断 PID 控制稳定 所用到的 阈值参数, 表示 频率档位 波动范围阈值

        PhaseWindow = 5; // 阶段检测 所用的 窗口/数据数量
        PhaseChangeThreshold = 0.1; // 监测阶段变化 以 重启优化 所用到的 阈值参数, 表示 执行向量 的变化百分比

        objective -= 0.005;

        StartTimeStamp = 0.0; // 开始的绝对时间戳
        initialized = 0;
        subscriber = 0;
        detachCupti = 0;
        tracingEnabled = 0;
        isMeasuring = 0;
        terminateThread = 0;
        istimeout = 0;
        RefCountAnalysis = 0;
        mutexFinalize = PTHREAD_MUTEX_INITIALIZER;
        mutexCondition = PTHREAD_COND_INITIALIZER;
    }

    GLOBAL_CONFIG() {
        init();
    }

};

// void globalInit(void);

#endif