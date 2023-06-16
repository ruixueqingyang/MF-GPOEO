/*******************************************************************************
Copyright(C), 2022-2023, 瑞雪轻飏
     FileName: DEPO.h
       Author: 瑞雪轻飏
      Version: 0.01
Creation Date: 20230412
  Description: DEPO 头文件 (Dynamic GPU power capping with online performance tracing for energy efficient GPU computing using DEPO tool)
       Others: 
*******************************************************************************/

#include "includes.h"

#include <cuda.h>
#include <cupti.h>
#include <cupti_target.h>
#include <cupti_profiler_target.h>

#include "global_var.h"
#include "Msg2EPRT.h"

#define STDCALL

#if defined(__cplusplus)
extern "C" {
#endif

MEASURED_DATA BaseData;
vector<MEASURED_DATA> vecMeasuredData;

int tmpCount;
CUcontext currCtx;
unordered_map<CUcontext, MEASURED_DATA> mapMeasuredData;

void callbackHandler(void * userdata, CUpti_CallbackDomain domain, CUpti_CallbackId cbid, void const * cbdata);
void measureData(MEASURED_DATA& Data, float MeasureDuration, float discount);
void measureBaseData(MEASURED_DATA& Data, float MeasureDuration, float discount);

// 计算 目标函数 和 性能损失
void ObjFunc(MEASURED_DATA& Data, MEASURED_DATA& BaseData, OPTIMIZATION_OBJECTIVE OptObj, float PerfLossConstraint);

enum GEAR_TYPE{PWR_LMT, SM_CLK_LMT, MEM_CLK_LMT};
static double PHI = ((double)1.0 + std::sqrt((double)5)) / 2 - 1;    // 0.618...
static double RESPHI = 1 - PHI;  // 0.382...
int GoldenSectionSearch(vector<int>& vecGear, GEAR_TYPE GearType, int Error);

void *DEPOMain(void *arg);

static bool injectionInitialized = false;
static void GlobalInit(void);
void show();
std::string trim(std::string s);
std::string erase_note(std::string s);
// extern 
int STDCALL InitializeInjection(void);
static void atExitHandler(void);
void registerAtExitHandler(void);

#if defined(__cplusplus)
}
#endif

// Helpful error handlers for standard CUPTI and CUDA runtime calls
#define CUPTI_API_CALL(apiFuncCall)                                            \
do {                                                                           \
    CUptiResult _status = apiFuncCall;                                         \
    if (_status != CUPTI_SUCCESS) {                                            \
        const char *errstr;                                                    \
        cuptiGetResultString(_status, &errstr);                                \
        fprintf(stderr, "%s:%d: error: function %s failed with error %s.\n",   \
                __FILE__, __LINE__, #apiFuncCall, errstr);                     \
        exit(EXIT_FAILURE);                                                    \
    }                                                                          \
} while (0)

#define RUNTIME_API_CALL(apiFuncCall)                                          \
do {                                                                           \
    cudaError_t _status = apiFuncCall;                                         \
    if (_status != cudaSuccess) {                                              \
        fprintf(stderr, "%s:%d: error: function %s failed with error %s.\n",   \
                __FILE__, __LINE__, #apiFuncCall, cudaGetErrorString(_status));\
        exit(EXIT_FAILURE);                                                    \
    }                                                                          \
} while (0)

#define MEMORY_ALLOCATION_CALL(var)                                            \
do {                                                                            \
    if (var == NULL) {                                                          \
        fprintf(stderr, "%s:%d: Error: Memory Allocation Failed \n",            \
                __FILE__, __LINE__);                                            \
        exit(EXIT_FAILURE);                                                     \
    }                                                                           \
} while (0)

#define DRIVER_API_CALL(apiFuncCall)                                           \
do {                                                                           \
    CUresult _status = apiFuncCall;                                            \
    if (_status != CUDA_SUCCESS) {                                             \
        fprintf(stderr, "%s:%d: error: function %s failed with error %d.\n",   \
                __FILE__, __LINE__, #apiFuncCall, _status);                    \
        exit(EXIT_FAILURE);                                                    \
    }                                                                          \
} while (0)

#define PTHREAD_CALL(call)                                                         \
do {                                                                               \
    int _status = call;                                                            \
    if (_status != 0) {                                                            \
        fprintf(stderr, "%s:%d: error: function %s failed with error code %d.\n",  \
                __FILE__, __LINE__, #call, _status);                               \
        exit(-1);                                                                  \
    }                                                                              \
} while (0)
