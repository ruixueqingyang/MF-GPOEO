/*******************************************************************************
Copyright(C), 2022-2022, 瑞雪轻飏
     FileName: analysis.h
       Author: 瑞雪轻飏
      Version: 0.01
Creation Date: 20221031
  Description: Exe/Gap 时间数据分析, 计算复合性能指标
       Others: 
*******************************************************************************/
#ifndef __ANALYSIS_H
#define __ANALYSIS_H

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

// 匹配到的 base数据和ctrl数据对
struct BASE_CTRL_EXE_PAIR {
    double StableIndex; // std; // 二者中的 最大 标准差指标
    double PerfIndex; // 性能损失
    KERNEL_INFO *pBase, *pCtrl; // base数据和ctrl数据 的指针
    // 如果调试需要, 这里可以加入别的信息, 例如kernel名称等
};

// 匹配到的 base数据和ctrl数据对
struct BASE_CTRL_GAP_PAIR {
    double StableIndex; // std; // 二者中的 最大 标准差指标
    double PerfIndex; // 性能损失
    GAP_INFO *pBase, *pCtrl; // base数据和ctrl数据 的指针
    // 如果调试需要, 这里可以加入别的信息, 例如kernel名称等
};

// 获得正负号
template <typename T> inline int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}

enum EXE_GAP_TIME
{
    EXE, GAP
};

double AnalysisKernelData(
    float& CoveredExeTimePct, float& MatchedExeTimePct, float& MatchedGapTimePct, 
    float& ExeIndex, float& GapIndex, unsigned int& MatchedStreams, float& UnMatchedStreamExeTimePct, 
    bool isDiscount, MEASURE_DATA& BaseData, MEASURE_DATA& CtrlData, 
    float ExeTimeRadius, float GapTimeRadius, OPTIMIZATION_OBJECTIVE OptObj, double PerfLossConstraint
);

#endif