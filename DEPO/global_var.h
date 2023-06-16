/*******************************************************************************
Copyright(C), 2022-2022, 瑞雪轻飏
     FileName: global_var.h
       Author: 瑞雪轻飏
      Version: 0.01
Creation Date: 20220614
  Description: 1. 声明从外部引入全部全局变量
               2.
       Others: //其他内容说明
*******************************************************************************/

#ifndef __GLOBAL_VAR_H
#define __GLOBAL_VAR_H

#include "includes.h"

#include <nvml.h>
#include <cupti.h>
#include <cuda.h>
#include <cuda_runtime.h>

#include "config.h"
#include "measure.h"
// #include "kernel_data.h"
// #include "analysis.h"

extern DEPO_CONFIG DEPOCfg; // 全局配置
extern RAW_DATA RawData; // 测得的原始数据

extern int OptGear;
extern int OptPwrGear; // 最优功率上限
extern int OptSMClkGear; // 最优核心频率
extern int OptMemClkGear; // 最优显存频率

// 进行实验时: 通过设置环境变量获得这两个参数
extern int TestedSMClk; // 评价 性能估计准确度 时, 使用的 SM 频率
extern int TestedMemClk; // 评价 性能估计准确度 时, 使用的 显存 频率
extern int TestedPwrLmt; // 评价 性能估计准确度 时, 使用的 功率上限

#define CHECK_TERMINATION_BREAK if (DEPOCfg.terminateThread) break
#define CHECK_TERMINATION_RETURN if (DEPOCfg.terminateThread) return

extern NVML_CONFIG NVMLConfig; // NVML 控制器
// extern MEASURE_DATA BaseData; // 基准数据的 数据库, 在默认配置下测量的数据会 融合/更新 到这里
extern int global_measure_count; // 全局采样计数
extern int global_data_index; // 全局数据 index 测量时写入 index 指向的数据缓冲空间
extern int CUPTIBufferCount; // CUPTI 缓存计数, 由 bufferRequested 函数 自增

// CUPTI 用户缓存管理
// extern CUPTI_BUFFER_MANAGER CUPTIBufferManager;

// 数据分析/处理线程的输入
extern std::vector<THREAD_ANALYSIS_ARGS> vecAnalysisArgs;

extern pthread_mutex_t mutexOutStream; // 文件输出流 的 锁

extern std::fstream DEPOOutStream; // 控制过程输出流

extern std::vector<int> vecPwrLmt;
extern std::vector<int> vecSMClk;
extern std::vector<int> vecMemClk; // 保存所有显存频率配置

#endif