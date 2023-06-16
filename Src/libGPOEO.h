/*******************************************************************************
Copyright(C), 2022-2022, 瑞雪轻飏
     FileName: libGPOEO.h
       Author: 瑞雪轻飏
      Version: 0.01
Creation Date: 20220615
  Description: 1. 定义 宏, 常量 等
               2. 
       Others: //其他内容说明
*******************************************************************************/

#ifndef __LIB_GPOEO_H
#define __LIB_GPOEO_H

#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <atomic>
#include <algorithm>
#include <cuda.h>
#include <cupti.h>
#include <algorithm>
#include <fstream>
#include <sstream>

#include "measure.h"
#include "kernel_data.h"
#include "global_var.h"
#include "is_stable.h"
#include "pid.h"
#include "analysis.h"
#include "config.h"
#include "Msg2EPRT.h"

#define STDCALL

#if defined(__cplusplus)
extern "C" {
#endif

// MAcros

// 输出文件路径
#define OUT_PATH "./MFGPOEO_out.txt"

// Function Declarations

static CUptiResult cuptiInitialize(void);

static void atExitHandler(void);

void CUPTIAPI callbackHandler(void *userdata, CUpti_CallbackDomain domain, CUpti_CallbackId cbid, void *cbInfo);

extern int STDCALL InitializeInjection(void);

int TuneMemClk(double& OptPerfIndex, double MeasureWindowDuration, int OptSMClkGear);

#if defined(__cplusplus)
}
#endif

#endif