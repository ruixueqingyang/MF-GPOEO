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

#include "includes.h"

#include <nvml.h>
#include <cupti.h>
#include <cuda.h>
#include <cuda_runtime.h>

#include "config.h"
#include "measure.h"
// #include "kernel_data.h"
// #include "analysis.h"

DEPO_CONFIG DEPOCfg; // 全局配置
RAW_DATA RawData; // 测得的原始数据

int OptGear;
int OptPwrGear; // 最优功率上限
int OptSMClkGear; // 最优核心频率
int OptMemClkGear; // 最优显存频率

// 进行实验时: 通过设置环境变量获得这两个参数
int TestedSMClk; // 评价 性能估计准确度 时, 使用的 SM 频率
int TestedMemClk; // 评价 性能估计准确度 时, 使用的 显存 频率
int TestedPwrLmt; // 评价 性能估计准确度 时, 使用的 功率上限

NVML_CONFIG NVMLConfig; // NVML 控制器
// MEASURE_DATA BaseData; // 基准数据的 数据库, 在默认配置下测量的数据会 融合/更新 到这里
int global_measure_count; // 全局采样计数
int global_data_index; // 全局数据 index 测量时写入 index 指向的数据缓冲空间
int CUPTIBufferCount; // CUPTI 缓存计数

// CUPTI 用户缓存管理
// CUPTI_BUFFER_MANAGER CUPTIBufferManager;

// 数据分析/处理线程的输入
std::vector<THREAD_ANALYSIS_ARGS> vecAnalysisArgs;

pthread_mutex_t mutexOutStream; // 文件输出流 的 锁

std::fstream DEPOOutStream; // 控制过程输出流

std::vector<int> vecPwrLmt;
std::vector<int> vecSMClk;
std::vector<int> vecMemClk; // 保存所有显存频率配置

#endif