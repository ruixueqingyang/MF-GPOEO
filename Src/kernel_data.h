/*******************************************************************************
Copyright(C), 2022-2022, 瑞雪轻飏
     FileName: kernel_data.h
       Author: 瑞雪轻飏
      Version: 0.01
Creation Date: 20220613
  Description: 1. 测得的 kernel activity 相关数据结构 及 数据分析函数
       Others: //其他内容说明
*******************************************************************************/

#ifndef __KERNEL_DATA_H
#define __KERNEL_DATA_H

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

#define LARGE_DOUBLE 1e100

// kernel名, kernel数量, kernel平均运行时间, kernel总运行时间
struct KERNEL_INFO {
public:
    int count;
    double avgExeTime;
    double sumExeTime;
    double minExeTime;
    double maxExeTime;
    double sumExeTime2;
    double stdExeTime;
    // D(X) = E(X^2) - (E(X))^2

    double GridSize, BlockSize;
    unsigned int GridRound, BlockRound;

    float ExeTimeStableIndex; // 执行时间稳定度指标

    std::string KernelID;

    KERNEL_INFO() {
        count = 0;
        avgExeTime = 0.0;
        sumExeTime = 0.0;
        sumExeTime2 = 0.0;
        minExeTime = LARGE_DOUBLE;
        maxExeTime = 0.0;
        stdExeTime = LARGE_DOUBLE;

        GridSize = 0.0;
        BlockSize = 0.0;
        GridRound = 0;
        BlockRound = 0;

        ExeTimeStableIndex = LARGE_DOUBLE;
        
        KernelID.clear();
    }
};

struct GAP_INFO {
public:
    int count;

    double GridSizePrev, BlockSizePrev;
    unsigned int GridRoundPrev, BlockRoundPrev;

    double GridSizePost, BlockSizePost;
    unsigned int GridRoundPost, BlockRoundPost;

    double avgGapTime;
    double sumGapTime;
    double minGapTime;
    double maxGapTime;
    double sumGapTime2;
    double stdGapTime; // D(X) = E(X^2) - (E(X))^2
    
    float GapTimeStableIndex; // gap 稳定度指标

    std::string GapID;

    GAP_INFO() {
        count = 0;
        GridSizePrev = 0.0;
        BlockSizePrev = 0.0;
        GridRoundPrev = 0;
        BlockRoundPrev = 0;

        GridSizePost = 0.0;
        BlockSizePost = 0.0;
        GridRoundPost = 0;
        BlockRoundPost = 0;

        avgGapTime = 0.0;
        sumGapTime = 0.0;
        sumGapTime2 = 0.0;
        minGapTime = LARGE_DOUBLE;
        maxGapTime = 0.0;
        stdGapTime = LARGE_DOUBLE;

        GapTimeStableIndex = LARGE_DOUBLE;

        GapID.clear();
    }
};

struct STREAM_INFO {
public:
    unsigned int KernelCount;
    double avgExeTime;
    double sumExeTime;
    double minExeTime;
    double maxExeTime;

    unsigned int GapCount;
    double avgGapTime;
    double sumGapTime;
    double minGapTime;
    double maxGapTime;

    uint64_t minStart;
    std::vector<uint64_t> vecStart;
    std::vector<uint64_t> vecEnd;

    STREAM_INFO() {
        KernelCount = 0;
        avgExeTime = 0.0;
        sumExeTime = 0.0;
        minExeTime = LARGE_DOUBLE;
        maxExeTime = 0.0;

        GapCount = 0;
        avgGapTime = 0.0;
        sumGapTime = 0.0;
        minGapTime = LARGE_DOUBLE;
        maxGapTime = 0.0;

        minStart = 0xffffffffffffffff;
    }

    void ResetMinMax() {
        minExeTime = LARGE_DOUBLE;
        maxExeTime = 0.0;
        minGapTime = LARGE_DOUBLE;
        maxGapTime = 0.0;
    }
};

// 用来保存一个测量间隔内测得的数据
class MEASURE_DATA {
public:

    std::vector<int> vecCUPTIBufferIndex; // 此次测量的 CUPTI 数据所存入的 buffer 的 index

    std::map< int, // streamID
        std::unordered_map< std::string, // KernelName-GridRound, BlockRound
                KERNEL_INFO > > map2Kernel;

    std::map< int, // streamID
        // KernelNamePrev-GridRoundPrev, BlockRoundPrev__KernelNamePost-GridRoundPost, BlockRoundPost
        std::unordered_map< std::string, 
                GAP_INFO > > map2Gap;

    std::map<int, STREAM_INFO> mapStream; // 保存每个 stream 中 所有 kernel 运行时间 的和

    int KernelKinds, NumKernels;
    float KernelsPerSec; // 单位时间 kernel 数量, 用来评估测量开销
    int GapKinds, NumGaps;
    double AccuExeTime, AccuGapTime;
    double UnCoveredExeTime, UnCoveredGapTime; // 没被数据库覆盖的时间
    double CoveredExeTime, CoveredGapTime; // 可以被数据库覆盖的时间
    // 总 Exe/Gap 时间分成两部分: 被数据库覆盖的 Covered, 不被数据库覆盖的 UnCovered; 是否覆盖 通过 grid/block size 判断
    // Covered 的 Exe/Gap 时间中的 数据分布集中的 可以进行匹配 记作 Matched; 是否匹配 通过 数据分布集中度指标 判断

    float avgExeTimePct, avgGapTimePct, maxExeTimePct;
    float maxStreamExeTimePct; // 各个 stream 中 sumExeTime 占 AccuExeTime 比例的 最大值

    unsigned int long long nvml_measure_count;
    float MeasureDuration, IdleTime;
    double TimeStamp;
    uint64_t CUPTIStartStamp, CUPTIEndStamp;

    std::map<int, std::string> mapKernelNamePrev, mapGridBlockSizePrev;
    std::map<int, double> mapGridSizePrev, mapBlockSizePrev;
    std::map<int, unsigned int> mapGridRoundPrev, mapBlockRoundPrev;
    std::map<int, uint64_t> mapEndStampPrev;

    // 测量时长, 能耗
    // double actualDuration; // 实际测量时长
    double Energy, avgPower; // W, J; 测量时长内总能耗, 平均功率
    double avgSMClk, avgMemClk; // MHz; 平均核心频率, 平均显存频率
    double avgGPUUtil, avgMemUtil; // %; 平均核心占用率, 平均显存IO占用率
    double maxPower, maxSMClk, maxMemClk, maxGPUUtil, maxMemUtil; // 各个数据的最大值
    unsigned int PowerLimit, SMClkUpperLimit, MemClkUpperLimit; // 本次测量期间功率配置: 功率上限, 核心频率上限, 显存频率上限
    double Obj; // 目标函数值
    double PerfLoss, ExeIndex, GapIndex;

    double sumSMClk, sumMemClk; // MHz
    double sumGPUUtil, sumMemUtil; // %

    void AnalysisCUPTIData();
    void ExtractKernelInfo(CUpti_Activity *record, int index);
    void CalculateAvgValue(float SampleInterval);
    void Analysis(double SystemTimeStamp, float SampleInterval, float in_MeasureDuration, unsigned int in_SMClkUpperLimit, unsigned int in_MemClkUpperLimit, bool cupti_flag) {
        TimeStamp = SystemTimeStamp;
        // MeasureDuration = in_MeasureDuration - IdleTime;
        MeasureDuration = in_MeasureDuration;
        SMClkUpperLimit = in_SMClkUpperLimit;
        MemClkUpperLimit = in_MemClkUpperLimit;
        if (cupti_flag) {
            AnalysisCUPTIData();
        }
        CalculateAvgValue(SampleInterval);
    }

    // 将新数据 融合 并 更新 到当前数据
    // 新数据, 衰减系数, Grid聚类半径, 执行时间聚类半径
    void Merge(MEASURE_DATA& NewData, float discount, bool isRelative, float ExeTimeRadius, float GapTimeRadius);

    // 将几个 MEASURE_DATA 数据结构 中的 NVML 数据求平均, 存到当前 MEASURE_DATA
    void AvgNVMLData(std::vector<MEASURE_DATA>& vecMeasureData, int begin, int num);

    void init() {
        vecCUPTIBufferIndex.clear();
        map2Kernel.clear();
        map2Gap.clear();
        mapStream.clear();
        KernelKinds = 0;
        NumKernels = 0;
        GapKinds = 0;
        NumGaps = 0;
        AccuExeTime = 0.0;
        AccuGapTime = 0.0;
        UnCoveredExeTime = 0.0;
        UnCoveredGapTime = 0.0;
        CoveredExeTime = 0.0;
        CoveredGapTime = 0.0;
        KernelsPerSec = 0.0;
        avgExeTimePct = 0.0;
        avgGapTimePct = 0.0;
        maxExeTimePct = 0.0;
        maxStreamExeTimePct = 0.0;

        mapKernelNamePrev.clear();
        mapGridBlockSizePrev.clear();
        mapEndStampPrev.clear();
        mapGridSizePrev.clear();
        mapBlockSizePrev.clear();
        mapGridRoundPrev.clear();
        mapBlockRoundPrev.clear();

        nvml_measure_count = 0;
        MeasureDuration = 0.0;
        IdleTime = 0.0;
        TimeStamp = 0.0;
        CUPTIStartStamp = 0;
        CUPTIEndStamp = 0;
        SMClkUpperLimit = 0;
        avgPower = 0.0; maxPower = 0.0; Energy = 0.0;
        avgSMClk = 0.0; maxSMClk = 0.0; sumSMClk = 0.0;
        avgMemClk = 0.0; maxMemClk = 0.0; sumMemClk = 0.0;
        avgMemUtil = 0.0; maxMemUtil = 0.0; sumMemUtil = 0.0;
        avgGPUUtil = 0.0; maxGPUUtil = 0.0; sumGPUUtil = 0.0;
        Obj = 1e9;
        PerfLoss = 1e9;
        ExeIndex = 1e9;
        GapIndex = 1e9;
    }

    void clear() {
        init();
    }

    MEASURE_DATA() {
        init();
    }

};

#endif