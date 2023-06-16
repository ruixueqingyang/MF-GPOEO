/*******************************************************************************
Copyright(C), 2022-2022, 瑞雪轻飏
     FileName: global_var.cpp
       Author: 瑞雪轻飏
      Version: 0.01
Creation Date: 20220624
  Description: 1. 定义全部全局变量
               2.
       Others: //其他内容说明
*******************************************************************************/

#ifndef __GLOBAL_VAR_MEASURE_H
#define __GLOBAL_VAR_MEASURE_H

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
#include <map>

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

#include "config.h"
#include "measure.h"
#include "kernel_data.h"
#include "analysis.h"

GLOBAL_CONFIG GlobalConfig; // 全局配置
float sumMeasurementTime; // 总 CUPTI 测量时间

int OptSMClkGear; // 最优核心频率
int OptMemClkGear; // 最优显存频率

// 进行实验时: 通过设置环境变量获得这两个参数
int TestedSMClk; // 评价 性能估计准确度 时, 使用的 SM 频率
int TestedMemClk; //  评价 性能估计准确度 时, 使用的 显存 频率

NVML_CONFIG NVMLConfig; // NVML 控制器
std::vector<MEASURE_DATA> vecMeasureData; // 采样数据数组/缓冲
MEASURE_DATA BaseData; // 基准数据的 数据库, 在默认配置下测量的数据会 融合/更新 到这里
std::vector<BASE_CTRL_EXE_PAIR> vecExeDataPair; // 匹配到的 base和ctrl Exe时间 数据对
std::vector<BASE_CTRL_GAP_PAIR> vecGapDataPair; // 匹配到的 base和ctrl Gap时间 数据对
int global_measure_count; // 全局采样计数
int global_data_index; // 全局数据 index 测量时写入 index 指向的数据缓冲空间
int CUPTIBufferCount; // CUPTI 缓存计数

// CUPTI 用户缓存管理
CUPTI_BUFFER_MANAGER CUPTIBufferManager;

// 数据分析/处理线程的输入
std::vector<THREAD_ANALYSIS_ARGS> vecAnalysisArgs;

pthread_mutex_t mutexOutStream; // 文件输出流 的 锁

std::fstream MFGPOEOOutStream; // 控制过程输出流
// uint64_t CUPTIStartStamp; // 当前 CUPTI 测量的开始时间戳

std::vector<unsigned int> vecSMClk;
std::vector<unsigned int> vecMemClk; // 保存所有显存频率配置

#endif