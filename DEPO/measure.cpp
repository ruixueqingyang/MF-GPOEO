/*******************************************************************************
Copyright(C), 2022-2023, 瑞雪轻飏
     FileName: measure.cpp
       Author: 瑞雪轻飏
      Version: 0.01
Creation Date: 20230413
  Description: CUPTI/NVML测量/调节相关函数和类
       Others: 
*******************************************************************************/
#include "measure.h"
#include "global_var.h"
// #include "is_stable.h"


void AlarmSampler(int signum) 
{

    // std::cout << "INFO: 进入 AlarmSampler" << std::endl;

    if(signum != SIGALRM) return;

    nvmlReturn_t nvmlResult;

    // gettimeofday(&MeasureData.currTimeStamp,NULL);

    // get current value
    unsigned int currPwr; // (mW)
    nvmlResult = nvmlDeviceGetPowerUsage(NVMLConfig.nvmlDevice, &currPwr);
    if (NVML_SUCCESS != nvmlResult) {
        printf("Failed to get power usage: %s\n", nvmlErrorString(nvmlResult));
        exit(-1);
    }

    unsigned int currSMClk; // (MHz)
    nvmlResult = nvmlDeviceGetClockInfo(NVMLConfig.nvmlDevice, NVML_CLOCK_SM, &currSMClk);
    if (NVML_SUCCESS != nvmlResult) {
        printf("Failed to get SM clock: %s\n", nvmlErrorString(nvmlResult));
        exit(-1);
    }

    unsigned int currMemClk; // (MHz)
    nvmlResult = nvmlDeviceGetClockInfo(NVMLConfig.nvmlDevice, NVML_CLOCK_MEM, &currMemClk);
    if (NVML_SUCCESS != nvmlResult) {
        printf("Failed to get memory clock: %s\n", nvmlErrorString(nvmlResult));
        exit(-1);
    }
    
    nvmlUtilization_t currUtil; // (%)
    nvmlResult = nvmlDeviceGetUtilizationRates(NVMLConfig.nvmlDevice, &currUtil);
    if (NVML_SUCCESS != nvmlResult) {
        printf("Failed to get utilization rate: %s\n", nvmlErrorString(nvmlResult));
        exit(-1);
    }

    if (currUtil.gpu == 0) { // 如果 GPU 核心没有占用, 累加空闲时间, 不记录数据
        RawData.IdleTime += DEPOCfg.SampleInterval;
        return;
    }

    double& maxPwr = RawData.maxPwr;
    double& maxSMClk = RawData.maxSMClk;
    double& maxMemClk = RawData.maxMemClk;
    double& maxMemUtil = RawData.maxMemUtil;
    double& maxGPUUtil = RawData.maxGPUUtil;

    maxPwr = currPwr > maxPwr ? currPwr : maxPwr;
    maxSMClk = currSMClk > maxSMClk ? currSMClk : maxSMClk;
    maxMemClk = currMemClk > maxMemClk ? currMemClk : maxMemClk;
    maxMemUtil = currUtil.memory > maxMemUtil ? currUtil.memory : maxMemUtil;
    maxGPUUtil = currUtil.gpu > maxGPUUtil ? currUtil.gpu : maxGPUUtil;

    // accumulate values
    RawData.sumPwr += currPwr / 1000;
    // std::cout << "measureData: RawData.sumPwr = " << RawData.sumPwr << std::endl;
    RawData.sumSMClk += currSMClk;
    RawData.sumMemClk += currMemClk;
    RawData.sumGPUUtil += currUtil.gpu;
    RawData.sumMemUtil += currUtil.memory;

    // update nvml_measure_count
    RawData.nvml_measure_count++;
    
}

void printTimeStamp() {
    struct timeval tmpTimeStamp;
    gettimeofday(&tmpTimeStamp, NULL);
    double RelativeTimeStamp = (double)tmpTimeStamp.tv_sec + (double)tmpTimeStamp.tv_usec * 1e-6 - DEPOCfg.StartTimeStamp;
    DEPOOutStream << std::endl;
    DEPOOutStream << "Relative Time Stamp: " << std::fixed << std::setprecision(2) << RelativeTimeStamp << " s" << std::endl;
}

void printSMClkUpperLimit(unsigned int SMClkUpperLimit) {
    printTimeStamp();
    DEPOOutStream << "SMClkUpperLimit: " << SMClkUpperLimit << std::endl;
}

void printMemClkUpperLimit(unsigned int MemClk) {
    printTimeStamp();
    DEPOOutStream << "MemClkUpperLimit: " << MemClk << std::endl;
}

void NVML_CONFIG::SetSMClkRange(unsigned int LowerLimit, unsigned int UpperLimit) {
    // 调整 内存频率上限
    nvmlReturn_t nvmlResult;
    nvmlResult = nvmlDeviceSetGpuLockedClocks(nvmlDevice, LowerLimit, UpperLimit);
    if (NVML_SUCCESS != nvmlResult) {
        printf("Failed to set SM clock range: %s\n", nvmlErrorString(nvmlResult));
        exit(-1);
    }
    printTimeStamp();
    DEPOOutStream << "SetSMClkRange: [ " << std::dec << LowerLimit << ", " << UpperLimit << " ] MHz" << std::endl;
    std::cout << "SetSMClkRange: [ " << std::dec << LowerLimit << ", " << UpperLimit << " ] MHz" << std::endl;
}

void NVML_CONFIG::SetSMClkUpperLimit(unsigned int SMClkLimit) {
    // 调整 核心频率上限
    nvmlReturn_t nvmlResult;
    nvmlResult = nvmlDeviceSetGpuLockedClocks(nvmlDevice, 0, SMClkLimit);
    if (NVML_SUCCESS != nvmlResult) {
        printf("Failed to set SM clock range: %s\n", nvmlErrorString(nvmlResult));
        exit(-1);
    }
    printTimeStamp();
    DEPOOutStream << "SetSMClkUpperLimit: " << std::dec << SMClkLimit << " MHz" << std::endl;
    std::cout << "SetSMClkUpperLimit: " << std::dec << SMClkLimit << " MHz" << std::endl;
}

void NVML_CONFIG::ResetSMClkLimit() {
    // 重置 核心频率上下限
    nvmlReturn_t nvmlResult;
    nvmlResult = nvmlDeviceResetGpuLockedClocks(nvmlDevice);
    if (NVML_SUCCESS != nvmlResult) {
        printf("Failed to reset SM clock range: %s\n", nvmlErrorString(nvmlResult));
        exit(-1);
    }
    printTimeStamp();
    DEPOOutStream << "ResetSMClkLimit" << std::endl;
    std::cout << "ResetSMClkLimit" << std::endl;
}

void NVML_CONFIG::SetMemClkRange(unsigned int LowerLimit, unsigned int UpperLimit) {
#if defined(SUPPORT_SET_MEMORY_CLOCK)
    // 调整 内存频率上限
    nvmlReturn_t nvmlResult;
    nvmlResult = nvmlDeviceSetMemoryLockedClocks(nvmlDevice, LowerLimit, UpperLimit);
    if (NVML_SUCCESS != nvmlResult) {
        printf("Failed to set Mem clock range: %s\n", nvmlErrorString(nvmlResult));
        exit(-1);
    }
    printTimeStamp();
    DEPOOutStream << "SetMemClkRange: [ " << std::dec << LowerLimit << ", " << UpperLimit << " ] MHz" << std::endl;
    std::cout << "SetMemClkRange: [ " << std::dec << LowerLimit << ", " << UpperLimit << " ] MHz" << std::endl;
#endif
}

void NVML_CONFIG::SetMemClkUpperLimit(unsigned int MemClkLimit) {
#if defined(SUPPORT_SET_MEMORY_CLOCK)
    // 调整 内存频率上限
    nvmlReturn_t nvmlResult;
    nvmlResult = nvmlDeviceSetMemoryLockedClocks(nvmlDevice, 0, MemClkLimit);
    if (NVML_SUCCESS != nvmlResult) {
        printf("Failed to set Mem clock upper limit: %s\n", nvmlErrorString(nvmlResult));
        exit(-1);
    }
    printTimeStamp();
    DEPOOutStream << "SetMemClkUpperLimit: " << std::dec << MemClkLimit << " MHz" << std::endl;
    std::cout << "SetMemClkUpperLimit: " << std::dec << MemClkLimit << " MHz" << std::endl;
#endif
}

void NVML_CONFIG::ResetMemClkLimit() {
#if defined(SUPPORT_SET_MEMORY_CLOCK)
    // 重置 内存频率上下限
    nvmlReturn_t nvmlResult;
    nvmlResult = nvmlDeviceResetMemoryLockedClocks(nvmlDevice);
    if (NVML_SUCCESS != nvmlResult) {
        printf("Failed to reset Mem clock range: %s\n", nvmlErrorString(nvmlResult));
        exit(-1);
    }
    printTimeStamp();
    DEPOOutStream << "ResetMemClkLimit" << std::endl;
    std::cout << "ResetMemClkLimit" << std::endl;
#endif
}

void NVML_CONFIG::SetPowerLimit(int PowerLimit) {
    // 调整能耗配置 (功率上限)
    nvmlReturn_t nvmlResult;
    nvmlResult = nvmlDeviceSetPowerManagementLimit(nvmlDevice, PowerLimit * 1e3);
    if (NVML_SUCCESS != nvmlResult) {
        printf("Failed to set power limit: %s\n", nvmlErrorString(nvmlResult));
        exit(-1);
    }
    printTimeStamp();
    DEPOOutStream << "SetPowerLimit: " << std::dec << PowerLimit << " W" << std::endl;
    std::cout << "SetPowerLimit: " << std::dec << PowerLimit << " W" << std::endl;
}

void NVML_CONFIG::ResetPowerLimit() {
    // 重置 功率上限
    nvmlReturn_t nvmlResult;
    unsigned int defaultLimit;
    nvmlResult = nvmlDeviceGetPowerManagementDefaultLimit(nvmlDevice, &defaultLimit);
    if (NVML_SUCCESS != nvmlResult) {
        printf("Failed to get default power limit: %s\n", nvmlErrorString(nvmlResult));
        exit(-1);
    }
    nvmlResult = nvmlDeviceSetPowerManagementLimit(nvmlDevice, defaultLimit);
    if (NVML_SUCCESS != nvmlResult) {
        printf("Failed to reset power limit: %s\n", nvmlErrorString(nvmlResult));
        exit(-1);
    }
    printTimeStamp();
    DEPOOutStream << "ResetPowerLimit" << std::endl;
    std::cout << "ResetPowerLimit" << std::endl;
}
