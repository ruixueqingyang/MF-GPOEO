/*******************************************************************************
Copyright(C), 2022-2022, 瑞雪轻飏
     FileName: kernel_data.cpp
       Author: 瑞雪轻飏
      Version: 0.01
Creation Date: 20220613
  Description: 测得的 kernel activity 相关数据结构 及 数据分析函数
       Others: 
*******************************************************************************/

#include "kernel_data.h"
#include "global_var.h"
#include "analysis.h"

// 分析 CUPTI 收集到的数据, 需要启动新线程
void MEASURE_DATA::AnalysisCUPTIData() {

    std::cout << "AnalysisCUPTIData: 进入" << std::endl;
    std::cout << "AnalysisCUPTIData: vecCUPTIBufferIndex.size() = " << std::dec << vecCUPTIBufferIndex.size() << std::endl;

    if (vecCUPTIBufferIndex.size() == 0) {
        return;
    }

    int tmpIndex;
    tmpIndex = vecCUPTIBufferIndex.front();
    CUPTIStartStamp = CUPTIBufferManager.vecBuf[tmpIndex].StartTimeStamp;
    tmpIndex = vecCUPTIBufferIndex.back();
    CUPTIEndStamp = CUPTIBufferManager.vecBuf[tmpIndex].EndTimeStamp;

    for (auto index : vecCUPTIBufferIndex) {

        std::cout << "AnalysisCUPTIData: index_buffer = " << std::dec << index << std::endl;
        std::cout << "AnalysisCUPTIData: ValidSize = " << std::dec << CUPTIBufferManager.vecBuf[index].ValidSize << std::endl;

        // 提取数据
        if (CUPTIBufferManager.vecBuf[index].ValidSize > 0) {
            CUptiResult status;
            CUpti_Activity *record = NULL; // 这个地址变量每次需要重置, 因为需要用该变量判断当前缓冲区的数据是否结束
            do {
                status = cuptiActivityGetNextRecord(CUPTIBufferManager.vecBuf[index].ptrAligned, CUPTIBufferManager.vecBuf[index].ValidSize, &record);
                // std::cout << "AnalysisCUPTIData: record = " << std::hex << (void*)(record) << std::endl;
                if (status == CUPTI_SUCCESS) {
                    ExtractKernelInfo(record, index);
                } else if (status == CUPTI_ERROR_MAX_LIMIT_REACHED) {
                    // std::cout << "AnalysisCUPTIData: CUPTI_ERROR_MAX_LIMIT_REACHED" << std::endl;
                    break;
                } else {
                    std::cout << "AnalysisCUPTIData: 错误" << std::endl;
                    break;
                    // continue;
                    // CUPTI_CALL(status);
                    // exit(1);
                }
            } while (1);
        }

        // 将提取数据后的缓冲区 标记为空
        // CUPTI_BUFFER_HEAD* pMeasureHead = (CUPTI_BUFFER_HEAD*)(CUPTIBufferManager.vecBuf[index].ptrAligned) - 1;
        // pMeasureHead->isEmpty = false;
        CUPTIBufferManager.releaseBuffer(index);
        // std::cout << "AnalysisCUPTIData: 缓冲区标记为空 index_buffer = " << std::dec << index << std::endl;
    }

    vecCUPTIBufferIndex.clear(); // 清空缓存 index

    std::cout << "AnalysisCUPTIData: done" << std::endl;
}

// 提取 kernel 信息
void MEASURE_DATA::ExtractKernelInfo(CUpti_Activity *record, int index) 
{
    // std::cout << "ExtractKernelInfo: 进入" << std::endl;
    // std::cout << "ExtractKernelInfo: index = " << std::dec << index << std::endl;
    switch (record->kind)
    {
        case CUPTI_ACTIVITY_KIND_KERNEL:
        case CUPTI_ACTIVITY_KIND_CONCURRENT_KERNEL:
        { // 时间单位是 ns

            // 这里处理版本兼容性
            // CUpti_ActivityKernel5: The activity record for a kernel (CUDA 11.0(with sm_80 support) onwards). (deprecated in CUDA 11.2)
            // CUpti_ActivityKernel6: The activity record for kernel. (deprecated in CUDA 11.6)
            // CUpti_ActivityKernel7
            CUpti_ActivityKernel6 *kernel = (CUpti_ActivityKernel6 *) record;
            // CUpti_ActivityKernel7 *kernel = (CUpti_ActivityKernel7 *) record;

            // 如果没有记录运行时间, 就舍弃当前数据
            if (kernel->start==0 || kernel->end==0) {
                // std::cout << "ExtractKernelInfo: no timestamp" << std::endl;
                return;
            }

            std::string KernelName = kernel->name;
            int StreamID = kernel->streamId;
            double tmpExeTime = (kernel->end - kernel->start) * 1e-9;

            mapStream[StreamID].KernelCount++;
            mapStream[StreamID].sumExeTime += tmpExeTime;

            mapStream[StreamID].minExeTime = std::min(tmpExeTime, mapStream[StreamID].minExeTime);
            mapStream[StreamID].maxExeTime = std::max(tmpExeTime, mapStream[StreamID].maxExeTime);
            mapStream[StreamID].minStart = std::min(kernel->start, mapStream[StreamID].minStart);
            // mapStream[StreamID].vecStart.emplace_back((uint64_t)kernel->start);
            // mapStream[StreamID].vecEnd.emplace_back((uint64_t)kernel->end);

            // 使用新数据结构 map2Kernel
            unsigned int GridSize = kernel->gridX * kernel->gridY * kernel->gridZ;
            unsigned int BlockSize = kernel->blockX * kernel->blockY * kernel->blockZ;
            unsigned int GridRound = std::ceil((double)GridSize * (double)BlockSize / (double)GlobalConfig.NumCores) * GlobalConfig.NumCores / BlockSize;
            unsigned int BlockRound = BlockSize;

            std::string GridBlockSize = std::to_string(GridSize) + ", " + std::to_string(BlockSize);
            std::string KernelID = KernelName + "-(" + std::to_string(GridRound) + ", " + std::to_string(BlockRound) + ")";

            // 新数据结构 map2Kernel
            if (map2Kernel[StreamID][KernelID].count == 0) {
                map2Kernel[StreamID][KernelID].GridSize = GridSize;
                map2Kernel[StreamID][KernelID].BlockSize = BlockSize;
                map2Kernel[StreamID][KernelID].GridRound = GridRound;
                map2Kernel[StreamID][KernelID].BlockRound = BlockRound;
                map2Kernel[StreamID][KernelID].KernelID = KernelID;
            }
            map2Kernel[StreamID][KernelID].count++;
            map2Kernel[StreamID][KernelID].sumExeTime += tmpExeTime;
            map2Kernel[StreamID][KernelID].sumExeTime2 += std::pow(tmpExeTime, 2);
            map2Kernel[StreamID][KernelID].minExeTime = std::min(tmpExeTime, map2Kernel[StreamID][KernelID].minExeTime);
            map2Kernel[StreamID][KernelID].maxExeTime = std::max(tmpExeTime, map2Kernel[StreamID][KernelID].maxExeTime);

            double tmpGapTime = 1e14;
            if (mapEndStampPrev.find(StreamID) != mapEndStampPrev.end() && mapEndStampPrev[StreamID] > 0) {

                std::string KernelNamePrev = mapKernelNamePrev[StreamID];
                std::string GridBlockSizePrev = mapGridBlockSizePrev[StreamID];
                tmpGapTime = (double)((int64_t)kernel->start - (int64_t)mapEndStampPrev[StreamID]) * 1e-9;

                // 只有 较小 的 Gap 才记录
                if (mapEndStampPrev[StreamID] < kernel->start)
                {
                    mapStream[StreamID].GapCount++;
                    mapStream[StreamID].sumGapTime += tmpGapTime;
                    mapStream[StreamID].minGapTime = std::min(tmpGapTime, mapStream[StreamID].minGapTime);
                    mapStream[StreamID].maxGapTime = std::max(tmpGapTime, mapStream[StreamID].maxGapTime);

                    unsigned int GridRoundPrev = mapGridRoundPrev[StreamID];
                    unsigned int BlockRoundPrev = mapBlockRoundPrev[StreamID];
                    std::string GapName = KernelNamePrev + "-(" + std::to_string(GridRoundPrev) + ", " + std::to_string(BlockRoundPrev) + ")--" + KernelName + "-(" + std::to_string(GridRound) + ", " + std::to_string(BlockRound) + ")";
                    
                    if (map2Gap[StreamID][GapName].count == 0) {
                        map2Gap[StreamID][GapName].GridRoundPrev = GridRoundPrev;
                        map2Gap[StreamID][GapName].BlockRoundPrev = BlockRoundPrev;
                        map2Gap[StreamID][GapName].GridRoundPost = GridRound;
                        map2Gap[StreamID][GapName].BlockRoundPost = BlockRound;
                        map2Gap[StreamID][GapName].GapID = GapName;
                    }
                    map2Gap[StreamID][GapName].count++;
                    map2Gap[StreamID][GapName].sumGapTime += tmpGapTime;
                    map2Gap[StreamID][GapName].sumGapTime2 += std::pow(tmpGapTime, 2);
                    map2Gap[StreamID][GapName].minGapTime = std::min(tmpGapTime, map2Gap[StreamID][GapName].minGapTime);
                    map2Gap[StreamID][GapName].maxGapTime = std::max(tmpGapTime, map2Gap[StreamID][GapName].maxGapTime);
                }

            }

            // 保存本次的信息, 下次存入 本次的 Gap 信息
            mapKernelNamePrev[StreamID] = KernelName;
            mapGridBlockSizePrev[StreamID] = GridBlockSize;
            mapGridSizePrev[StreamID] = GridSize;
            mapBlockSizePrev[StreamID] = BlockSize;
            mapGridRoundPrev[StreamID] = GridRound;
            mapBlockRoundPrev[StreamID] = BlockRound;
            mapEndStampPrev[StreamID] = kernel->end;

            break;
        }
        default:
        {
            // printf("  <unknown>\n");
            break;
        }
    }
    // std::cout << "ExtractKernelInfo: done" << std::endl;
}

void MEASURE_DATA::CalculateAvgValue(float SampleInterval) {

    MFGPOEOOutStream << "CalculateAvgValue: nvml_measure_count = " << std::dec << nvml_measure_count << std::endl;
    // MFGPOEOOutStream << "CalculateAvgValue: sumMemClk = " << std::fixed << std::setprecision(2) << sumMemClk << std::endl;

    // 需要计算 总能耗 和 平均功率
    nvml_measure_count = (nvml_measure_count == 0) ? 1 : nvml_measure_count;
    avgPower = Energy / nvml_measure_count;
    Energy = Energy * SampleInterval;
    avgSMClk = sumSMClk / nvml_measure_count;
    avgMemClk = sumMemClk / nvml_measure_count;
    avgGPUUtil = sumGPUUtil / nvml_measure_count;
    avgMemUtil = sumMemUtil / nvml_measure_count;

    // MFGPOEOOutStream << "CalculateAvgValue: avgSMClk = " << std::fixed << std::setprecision(2) << avgSMClk << " MHz" << std::endl;

    KernelKinds = 0;
    NumKernels = 0;
    AccuExeTime = 0.0;
    AccuGapTime = 0.0;
    double maxExeTime = 0.0;
    maxExeTimePct = 0.0;
    for (auto& data : mapStream) 
    {
        data.second.avgExeTime = data.second.sumExeTime / data.second.KernelCount;
        NumKernels += data.second.KernelCount;
        AccuExeTime += data.second.sumExeTime;
        maxExeTime = std::max((double)maxExeTime, (double)data.second.sumExeTime);
        maxExeTimePct = std::max((float)maxExeTimePct, (float)(data.second.sumExeTime / MeasureDuration));
        data.second.avgGapTime = data.second.sumGapTime / data.second.GapCount;
    }
    if (AccuExeTime >= 1e-9)
    {
        maxStreamExeTimePct = maxExeTime / AccuExeTime;
    }
    
    for (auto& pairMapKernel : map2Kernel) {
        int StreamID = pairMapKernel.first;
        for (auto& pairKernel : pairMapKernel.second) {
            std::string KernelID = pairKernel.first;
            KernelKinds++;
            // NumKernels += pairKernel.second.count;
            // AccuExeTime += pairKernel.second.sumExeTime;

            if (pairKernel.second.count > 0) {
                pairKernel.second.avgExeTime = pairKernel.second.sumExeTime / pairKernel.second.count;
                pairKernel.second.stdExeTime = std::sqrt(pairKernel.second.sumExeTime2 / pairKernel.second.count - std::pow(pairKernel.second.avgExeTime, 2));
                pairKernel.second.ExeTimeStableIndex = pairKernel.second.stdExeTime / pairKernel.second.avgExeTime;
            }
        }
    }

    // MFGPOEOOutStream << "BaseData.maxExeTimePct = " << std::fixed << std::setprecision(3) << BaseData.maxExeTimePct << std::endl;

    GapKinds = 0;
    NumGaps = 0;
    AccuGapTime = 0.0;
    for (auto iterMapGap = map2Gap.begin(); iterMapGap != map2Gap.end(); iterMapGap++)
    {
        int StreamID = iterMapGap->first;
        for (auto iterGap = iterMapGap->second.begin(); iterGap != iterMapGap->second.end(); )
        {
            if (iterGap->second.count > 0) {
                iterGap->second.avgGapTime = iterGap->second.sumGapTime / iterGap->second.count;
                iterGap->second.stdGapTime = std::sqrt(iterGap->second.sumGapTime2 / iterGap->second.count - std::pow(iterGap->second.avgGapTime, 2));
                iterGap->second.GapTimeStableIndex = iterGap->second.stdGapTime / iterGap->second.avgGapTime;

                GapKinds++;
                NumGaps += iterGap->second.count;
                AccuGapTime += iterGap->second.sumGapTime;
                iterGap++;
            } else {
                iterGap = iterMapGap->second.erase(iterGap);
            }
        }
        
    }

    KernelsPerSec = (float)NumKernels / MeasureDuration;

    MFGPOEOOutStream << "CalculateAvgValue: MeasureDuration = " << std::fixed << std::setprecision(2) << MeasureDuration << std::endl;
    MFGPOEOOutStream << "CalculateAvgValue: NumKernels = " << std::dec << NumKernels << std::endl;
    MFGPOEOOutStream << "CalculateAvgValue: NumGaps = " << std::dec << NumGaps << std::endl;
    MFGPOEOOutStream << "CalculateAvgValue: KernelsPerSec = " << std::fixed << std::setprecision(1) << KernelsPerSec << std::endl;
    MFGPOEOOutStream << "CalculateAvgValue: avgGPUUtil = " << std::fixed << std::setprecision(3) << avgGPUUtil << std::endl;
    MFGPOEOOutStream << "CalculateAvgValue: avgMemUtil = " << std::fixed << std::setprecision(3) << avgMemUtil << std::endl;
    MFGPOEOOutStream << "CalculateAvgValue: IdleTime = " << std::fixed << std::setprecision(3) << IdleTime << std::endl;


    if (mapStream.size() > 0) {
        MFGPOEOOutStream << "CalculateAvgValue: AccuExeTime = " << std::fixed << std::setprecision(2) << AccuExeTime << std::endl;
        MFGPOEOOutStream << "CalculateAvgValue: AccuGapTime = " << std::fixed << std::setprecision(2) << AccuGapTime << std::endl;
        MFGPOEOOutStream << "CalculateAvgValue: mapStream.size() = " << std::dec << mapStream.size() << std::endl;
        avgExeTimePct = AccuExeTime / mapStream.size() / MeasureDuration;
        avgGapTimePct = AccuGapTime / mapStream.size() / MeasureDuration;
    }

    // MFGPOEOOutStream << "CalculateAvgValue: maxStreamExeTimePct = " << std::fixed << std::setprecision(2) << maxStreamExeTimePct * 100 << " %" << std::endl;
    // MFGPOEOOutStream << "CalculateAvgValue: avgExeTimePct = " << std::fixed << std::setprecision(2) << avgExeTimePct * 100 << " %" << std::endl;
}

// 将新数据 融合 并 更新 到当前数据
// 新数据, 衰减系数, Grid聚类半径, 执行时间聚类半径
void MEASURE_DATA::Merge(MEASURE_DATA& NewData, float discount, bool isRelative, float ExeTimeRadius, float GapTimeRadius) {

    // MFGPOEOOutStream << std::endl;
    std::cout << "Merge: 进入" << std::endl;
    discount = std::max((float)1e-2, discount);
    // discount = std::min((float)1.0, discount);

    // tmpDiscount * MeasureDuration / NewData.MeasureDuration == discount

    if (isRelative == true)
    {
        discount = discount * NewData.MeasureDuration / MeasureDuration;
    }

    // MFGPOEOOutStream << "Merge: MeasureDuration = " << std::fixed << std::setprecision(2) << MeasureDuration << " s" << std::endl;
    // MFGPOEOOutStream << "Merge: AccuExeTime = " << std::fixed << std::setprecision(2) << AccuExeTime << " s" << std::endl;
    // MFGPOEOOutStream << "Merge: NewData.MeasureDuration = " << std::fixed << std::setprecision(2) << NewData.MeasureDuration << " s" << std::endl;
    // MFGPOEOOutStream << "Merge: NewData.AccuExeTime = " << std::fixed << std::setprecision(2) << NewData.AccuExeTime << " s" << std::endl;

    // 需要计算 总能耗 和 平均功率
    Energy = NewData.Energy + discount * Energy;
    sumSMClk = NewData.sumSMClk + discount * sumSMClk;
    sumMemClk = NewData.sumMemClk + discount * sumMemClk;
    sumGPUUtil = NewData.sumGPUUtil + discount * sumGPUUtil;
    sumMemUtil = NewData.sumMemUtil + discount * sumMemUtil;

    avgPower = (avgPower * discount * nvml_measure_count + NewData.avgPower * NewData.nvml_measure_count) / (discount * nvml_measure_count + NewData.nvml_measure_count);
    nvml_measure_count = NewData.nvml_measure_count + discount * nvml_measure_count;
    avgSMClk = sumSMClk / nvml_measure_count;
    avgMemClk = sumMemClk / nvml_measure_count;
    avgGPUUtil = sumGPUUtil / nvml_measure_count;
    avgMemUtil = sumMemUtil / nvml_measure_count;

    IdleTime = NewData.IdleTime + discount * IdleTime;
    MeasureDuration = NewData.MeasureDuration + discount * MeasureDuration;
    KernelKinds += NewData.KernelKinds;
    NumKernels = NewData.NumKernels + discount * NumKernels;
    AccuExeTime = NewData.AccuExeTime + discount * AccuExeTime;
    
    if (MeasureDuration > 0.0) {
        KernelsPerSec = (float)NumKernels / MeasureDuration;
    }
    GapKinds += NewData.GapKinds;
    NumGaps = NewData.NumGaps + discount * NumGaps;
    AccuGapTime = NewData.AccuGapTime + discount * AccuGapTime;

    // 融合 mapStream 数据
    for (auto& data : NewData.mapStream) {
        int StreamID = data.first;

        mapStream[StreamID].KernelCount = data.second.KernelCount + discount * mapStream[StreamID].KernelCount;
        mapStream[StreamID].sumExeTime = data.second.sumExeTime + discount * mapStream[StreamID].sumExeTime;
        if (mapStream[StreamID].KernelCount > 0) {
            mapStream[StreamID].avgExeTime = mapStream[StreamID].sumExeTime / mapStream[StreamID].KernelCount;
        }
        if (mapStream[StreamID].KernelCount == data.second.KernelCount) {
            mapStream[StreamID].minExeTime = data.second.minExeTime;
            mapStream[StreamID].maxExeTime = data.second.maxExeTime;
        } else {
            mapStream[StreamID].minExeTime = std::min(mapStream[StreamID].minExeTime, data.second.minExeTime);
            mapStream[StreamID].maxExeTime = std::max(mapStream[StreamID].maxExeTime, data.second.maxExeTime);
        }

        mapStream[StreamID].GapCount = data.second.GapCount + discount * mapStream[StreamID].GapCount;
        mapStream[StreamID].sumGapTime = data.second.sumGapTime + discount * mapStream[StreamID].sumGapTime;
        if (mapStream[StreamID].GapCount > 0) {
            mapStream[StreamID].avgGapTime = mapStream[StreamID].sumGapTime / mapStream[StreamID].GapCount;
        }
        if (mapStream[StreamID].GapCount == data.second.GapCount) {
            mapStream[StreamID].minGapTime = data.second.minGapTime;
            mapStream[StreamID].maxGapTime = data.second.maxGapTime;
        } else {
            mapStream[StreamID].minGapTime = std::min(mapStream[StreamID].minGapTime, data.second.minGapTime);
            mapStream[StreamID].maxGapTime = std::max(mapStream[StreamID].maxGapTime, data.second.maxGapTime);
        }
    }
    maxExeTimePct = 0.0;
    double maxExeTime = 0.0;
    for (auto& data : mapStream) 
    {
        maxExeTime = std::max((double)maxExeTime, (double)data.second.sumExeTime);
        maxExeTimePct = std::max((float)maxExeTimePct, (float)(data.second.sumExeTime / MeasureDuration));
    }
    if (AccuExeTime > 0.0)
    {
        maxStreamExeTimePct = maxExeTime / AccuExeTime;
    }
    if (mapStream.size() > 0) {
        avgExeTimePct = AccuExeTime / mapStream.size() / MeasureDuration;
        avgGapTimePct = AccuGapTime / mapStream.size() / MeasureDuration;
    }

    // 融合 map2Kernel 数据
    for (auto& pairMapKernel : NewData.map2Kernel) {
        int StreamID = pairMapKernel.first;
        auto& mapKernelBase = map2Kernel[StreamID];

        for (auto& pairKernel : pairMapKernel.second) {
            if (pairKernel.second.count == 0) {
                continue;
            }
            std::string KernelID = pairKernel.first;

            if (mapKernelBase[KernelID].count == 0) { // 如果是 0, 说明新建了项
                mapKernelBase[KernelID] = pairKernel.second; // 直接赋值不衰减
                continue;
            }

            // 取整计算衰减系数
            double tmpDiscount = discount;
            int tmpCount;
            tmpCount = std::round(mapKernelBase[KernelID].count * tmpDiscount);
            // 根据取整的数量, 重新确定 tmpDiscount
            tmpDiscount = (double)tmpCount / mapKernelBase[KernelID].count;

            int tmpSumCount = pairKernel.second.count + tmpCount;
            if (tmpSumCount <= 0) {
                continue;
            }
            double tmpSumExe = pairKernel.second.sumExeTime + mapKernelBase[KernelID].sumExeTime * tmpDiscount;
            double tmpSumExe2 = pairKernel.second.sumExeTime2 + mapKernelBase[KernelID].sumExeTime2 * tmpDiscount;
            double tmpAvgExe = tmpSumExe / tmpSumCount;
            // 计算融合后的 执行时间波动指标
            double tmpStdExeTime = pairKernel.second.ExeTimeStableIndex;
            double D = tmpSumExe2 / tmpSumCount - std::pow(tmpAvgExe, 2);
            // if (D >= 0.0) {
                tmpStdExeTime = std::sqrt(D);
            // }
            float tmpExeTimeStableIndex = tmpStdExeTime / tmpAvgExe;

            mapKernelBase[KernelID].minExeTime = std::min(pairKernel.second.minExeTime, mapKernelBase[KernelID].minExeTime);
            mapKernelBase[KernelID].maxExeTime = std::max(pairKernel.second.maxExeTime, mapKernelBase[KernelID].maxExeTime);
            mapKernelBase[KernelID].count = tmpSumCount;
            mapKernelBase[KernelID].sumExeTime = tmpSumExe;
            mapKernelBase[KernelID].sumExeTime2 = tmpSumExe2;
            mapKernelBase[KernelID].avgExeTime = tmpAvgExe;
            mapKernelBase[KernelID].stdExeTime = tmpStdExeTime;
            mapKernelBase[KernelID].ExeTimeStableIndex = tmpExeTimeStableIndex;
            
        }
        
    }

    // 融合 map2Gap 数据
    for (auto& pairMapGap : NewData.map2Gap) {
        int StreamID = pairMapGap.first;
        auto& mapGapBase = map2Gap[StreamID];

        for (auto& pairGap : pairMapGap.second) {
            if (pairGap.second.count == 0) {
                continue;
            }
            std::string GapName = pairGap.first;

            if (mapGapBase[GapName].count == 0) { // 如果是 0, 说明新建了项
                mapGapBase[GapName] = pairGap.second; // 直接赋值不衰减
                continue;
            }

            // 取整计算衰减系数
            double tmpDiscount = discount;
            int tmpCount;
            tmpCount = std::round(mapGapBase[GapName].count * tmpDiscount);
            // 根据取整的数量, 重新确定 tmpDiscount
            tmpDiscount = (double)tmpCount / mapGapBase[GapName].count;

            int tmpSumCount = pairGap.second.count + tmpCount;
            if (tmpSumCount <= 0) {
                continue;
            }
            double tmpSumGap = pairGap.second.sumGapTime + mapGapBase[GapName].sumGapTime * tmpDiscount;
            double tmpSumGap2 = pairGap.second.sumGapTime2 + mapGapBase[GapName].sumGapTime2 * tmpDiscount;
            double tmpAvgGap = tmpSumGap / tmpSumCount;
            // 计算融合后的 执行时间波动指标
            // double tmpStdGapTime = std::sqrt(tmpSumGap2 / tmpSumCount - std::pow(tmpAvgGap, 2));
            double tmpStdGapTime = pairGap.second.GapTimeStableIndex;
            double D = tmpSumGap2 / tmpSumCount - std::pow(tmpAvgGap, 2);
            // if (D >= 0.0) {
                tmpStdGapTime = std::sqrt(D);
            // }
            float tmpGapTimeStableIndex = tmpStdGapTime / tmpAvgGap;

            mapGapBase[GapName].minGapTime = std::min(pairGap.second.minGapTime, mapGapBase[GapName].minGapTime);
            mapGapBase[GapName].maxGapTime = std::max(pairGap.second.maxGapTime, mapGapBase[GapName].maxGapTime);
            mapGapBase[GapName].count = tmpSumCount;
            mapGapBase[GapName].sumGapTime = tmpSumGap;
            mapGapBase[GapName].sumGapTime2 = tmpSumGap2;
            mapGapBase[GapName].avgGapTime = tmpAvgGap;
            mapGapBase[GapName].stdGapTime = tmpStdGapTime;
            mapGapBase[GapName].GapTimeStableIndex = tmpGapTimeStableIndex;
        }
    }

    MFGPOEOOutStream << "Merge: MeasureWindowDuration = " << std::fixed << std::setprecision(2) << MeasureDuration << " s" << std::endl;
    MFGPOEOOutStream << "Merge: IdleTime = " << std::fixed << std::setprecision(2) << IdleTime << " s" << std::endl;
    MFGPOEOOutStream << "Merge: maxStreamExeTimePct = " << std::fixed << std::setprecision(2) << maxStreamExeTimePct * 100 << " %" << std::endl;
    MFGPOEOOutStream << "Merge: maxExeTimePct = " << std::fixed << std::setprecision(2) << maxExeTimePct * 100 << " %" << std::endl;
    MFGPOEOOutStream << "Merge: avgExeTimePct = " << std::fixed << std::setprecision(2) << avgExeTimePct * 100 << " %" << std::endl;
    MFGPOEOOutStream << "Merge: avgGapTimePct = " << std::fixed << std::setprecision(2) << avgGapTimePct * 100 << " %" << std::endl;
    MFGPOEOOutStream << "Merge: avgGPUUtil = " << std::fixed << std::setprecision(2) << avgGPUUtil << " %" << std::endl;
    MFGPOEOOutStream << "Merge: avgMemUtil = " << std::fixed << std::setprecision(2) << avgMemUtil << " %" << std::endl;
    std::cout << "Merge: done" << std::endl;
}

// 将几个 MEASURE_DATA 数据结构 中的 NVML 数据求平均, 存到当前 MEASURE_DATA
void MEASURE_DATA::AvgNVMLData(std::vector<MEASURE_DATA>& vecMeasureData, int begin, int num) {
    avgPower = 0.0;
    avgSMClk = 0.0;
    avgMemClk = 0.0;
    avgMemUtil = 0.0;
    avgGPUUtil = 0.0;
    for (int i = 0; i < num; ++i) {
        int index = (i + begin) % vecMeasureData.size();
        
        avgPower += vecMeasureData[index].avgPower;
        avgSMClk += vecMeasureData[index].avgSMClk;
        avgMemClk += vecMeasureData[index].avgMemClk;
        avgMemUtil += vecMeasureData[index].avgMemUtil;
        avgGPUUtil += vecMeasureData[index].avgGPUUtil;
    }

    avgPower /= num;
    avgSMClk /= num;
    avgMemClk /= num;
    avgMemUtil /= num;
    avgGPUUtil /= num;
}
