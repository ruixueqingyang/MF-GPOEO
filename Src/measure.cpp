/*******************************************************************************
Copyright(C), 2022-2022, 瑞雪轻飏
     FileName: measure.cpp
       Author: 瑞雪轻飏
      Version: 0.01
Creation Date: 20221024
  Description: CUPTI/NVML测量/调节相关函数和类
       Others: 
*******************************************************************************/
#include "measure.h"
#include "global_var.h"
#include "is_stable.h"

// 这个函数要尽量快, 会增加测量开销
void CUPTIAPI bufferRequested(uint8_t **buffer, size_t *size, size_t *maxNumRecords)
{
    // CUPTIBufferCount++;
    // int index_buffer = CUPTIBufferCount%VECTOR_MEASURE_BUFFER_SIZE;
    int index_buffer = CUPTIBufferManager.getValidBufferIndex();

    *size = CUPTIBufferManager.vecBuf[index_buffer].BufSize;
    *buffer = CUPTIBufferManager.vecBuf[index_buffer].ptrAligned;
    *maxNumRecords = 0;

    // *size = CUPTI_BUF_SIZE;
    // *buffer = vecCUPTIBufferAligned[index_buffer]; // 使用外部预分配的空间
    // *maxNumRecords = 0;
    // 填充缓冲数据头信息
    // CUPTI_BUFFER_HEAD* pMeasureHead = (CUPTI_BUFFER_HEAD*)(buffer) - 1;
    // if (pMeasureHead->isEmpty == true) {
    //     pMeasureHead->isEmpty = false;
    // } else {
    //     std::cout << "bufferRequested: isEmpty == false 缓冲队列已满" << std::endl;
    //     exit(0);
    // }
    CUPTI_CALL(cuptiGetTimestamp(&CUPTIBufferManager.vecBuf[index_buffer].StartTimeStamp));
    // 将缓冲数据的 index 存入 当前次测量的缓冲数据索引 列表
    vecMeasureData[global_data_index].vecCUPTIBufferIndex.emplace_back(index_buffer);
    std::cout << "bufferRequested: TimeStamp = " << std::dec << CUPTIBufferManager.vecBuf[index_buffer].StartTimeStamp << std::endl;
    std::cout << "bufferRequested: global_data_index = " << std::dec << global_data_index << std::endl;
    std::cout << "bufferRequested: index_buffer = " << std::dec << index_buffer << std::endl;
    std::cout << "bufferRequested: buffer = " << std::hex << (void*)(*buffer) << std::endl;
}

// 这个函数要尽量快, 会增加测量开销, 因此只进行 buffer 拷贝
// 可以只进行拷贝, 具体处理交给其他线程
void CUPTIAPI bufferCompleted(CUcontext ctx, uint32_t streamId, uint8_t *buffer, size_t size, size_t validSize)
{
    CUptiResult status;
    CUpti_Activity *record = NULL;

    CUPTI_BUFFER_HEAD* pMeasureHead = (CUPTI_BUFFER_HEAD*)(buffer) - 1;
    int index_buffer = pMeasureHead->index;

    CUPTIBufferManager.vecBuf[index_buffer].Ctx = ctx;
    CUPTIBufferManager.vecBuf[index_buffer].StreamID = streamId;
    // CUPTIBufferManager.vecBuf[index_buffer].BufSize = size;
    CUPTIBufferManager.vecBuf[index_buffer].ValidSize = validSize;

    // vecCUPTICtx[index_buffer] = ctx;
    // vecCUPTIStreamID[index_buffer] = streamId;
    // vecCUPTIBufSize[index_buffer] = size;
    // vecCUPTIValidSize[index_buffer] = validSize;
    CUPTI_CALL(cuptiGetTimestamp(&CUPTIBufferManager.vecBuf[index_buffer].EndTimeStamp));
    // // 将缓冲数据的 index 存入 当前次测量的缓冲数据索引 列表
    // vecMeasureData[global_data_index].vecCUPTIBufferIndex.emplace_back(index_buffer);

    std::cout << "bufferCompleted: TimeStamp = " << std::dec << CUPTIBufferManager.vecBuf[index_buffer].EndTimeStamp << std::endl;
    std::cout << "bufferCompleted: index_buffer = " << std::dec << index_buffer << std::endl;
    std::cout << "bufferCompleted: buffer = " << std::hex << (void*)(buffer) << std::endl;
    std::cout << "bufferCompleted: validSize = " << std::dec << validSize << std::endl;

    if (validSize > 0) {
        // report any records dropped from the queue
        size_t dropped;
        CUPTI_CALL(cuptiActivityGetNumDroppedRecords(ctx, streamId, &dropped));
        if (dropped != 0) {
            printf("Dropped %u activity records\n", (unsigned int) dropped);
        }
    }

    std::cout << "bufferCompleted: done" << std::endl;
}

void AlarmSampler(int signum) 
{

    // std::cout << "INFO: 进入 AlarmSampler" << std::endl;

    if(signum != SIGALRM) return;

    nvmlReturn_t nvmlResult;

    // gettimeofday(&MeasureData.currTimeStamp,NULL);

    // get current value
    unsigned int currPower; // (mW)
    nvmlResult = nvmlDeviceGetPowerUsage(NVMLConfig.nvmlDevice, &currPower);
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
        vecMeasureData[global_data_index].IdleTime += (float)SAMPLE_INTERVAL / 1000;
        return;
    }

    double& maxPower = vecMeasureData[global_data_index].maxPower;
    double& maxSMClk = vecMeasureData[global_data_index].maxSMClk;
    double& maxMemClk = vecMeasureData[global_data_index].maxMemClk;
    double& maxMemUtil = vecMeasureData[global_data_index].maxMemUtil;
    double& maxGPUUtil = vecMeasureData[global_data_index].maxGPUUtil;

    maxPower = currPower > maxPower ? currPower : maxPower;
    maxSMClk = currSMClk > maxSMClk ? currSMClk : maxSMClk;
    maxMemClk = currMemClk > maxMemClk ? currMemClk : maxMemClk;
    maxMemUtil = currUtil.memory > maxMemUtil ? currUtil.memory : maxMemUtil;
    maxGPUUtil = currUtil.gpu > maxGPUUtil ? currUtil.gpu : maxGPUUtil;

    // accumulate values
    vecMeasureData[global_data_index].Energy += currPower / 1000;
    vecMeasureData[global_data_index].sumSMClk += currSMClk;
    vecMeasureData[global_data_index].sumMemClk += currMemClk;
    vecMeasureData[global_data_index].sumGPUUtil += currUtil.gpu;
    vecMeasureData[global_data_index].sumMemUtil += currUtil.memory;

    // update nvml_measure_count
    vecMeasureData[global_data_index].nvml_measure_count++;
    
}

void printTimeStamp() {
    struct timeval tmpTimeStamp;
    gettimeofday(&tmpTimeStamp, NULL);
    double RelativeTimeStamp = (double)tmpTimeStamp.tv_sec + (double)tmpTimeStamp.tv_usec * 1e-6 - GlobalConfig.StartTimeStamp;
    MFGPOEOOutStream << std::endl;
    MFGPOEOOutStream << "Relative Time Stamp: " << std::fixed << std::setprecision(2) << RelativeTimeStamp << " s" << std::endl;
}

void printSMClkUpperLimit(unsigned int SMClkUpperLimit) {
    printTimeStamp();
    MFGPOEOOutStream << "SMClkUpperLimit: " << SMClkUpperLimit << std::endl;
}

void printMemClkUpperLimit(unsigned int MemClk) {
    printTimeStamp();
    MFGPOEOOutStream << "MemClkUpperLimit: " << MemClk << std::endl;
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
    MFGPOEOOutStream << "SetSMClkRange: [ " << std::dec << LowerLimit << ", " << UpperLimit << " ] MHz" << std::endl;
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
    MFGPOEOOutStream << "SetSMClkUpperLimit: " << std::dec << SMClkLimit << " MHz" << std::endl;
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
    MFGPOEOOutStream << "ResetSMClkLimit" << std::endl;
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
    MFGPOEOOutStream << "SetMemClkRange: [ " << std::dec << LowerLimit << ", " << UpperLimit << " ] MHz" << std::endl;
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
    MFGPOEOOutStream << "SetMemClkUpperLimit: " << std::dec << MemClkLimit << " MHz" << std::endl;
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
    MFGPOEOOutStream << "ResetMemClkLimit" << std::endl;
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
    MFGPOEOOutStream << "SetPowerLimit: " << std::dec << PowerLimit << " W" << std::endl;
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
    MFGPOEOOutStream << "ResetPowerLimit" << std::endl;
    std::cout << "ResetPowerLimit" << std::endl;
}
