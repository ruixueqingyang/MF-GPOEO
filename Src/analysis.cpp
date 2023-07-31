/*******************************************************************************
Copyright(C), 2022-2022, 瑞雪轻飏
     FileName: analysis.cpp
       Author: 瑞雪轻飏
      Version: 0.01
Creation Date: 20221031
  Description: Exe/Gap 时间数据分析, 计算复合性能指标
       Others: 
*******************************************************************************/

#include "global_var.h"
#include "analysis.h"

// 向 vec 插入 数据, 按照 PerfIndex 降序排列
void CmpPerf_Insert(
    std::vector<BASE_CTRL_EXE_PAIR>& vec, 
    BASE_CTRL_EXE_PAIR& newData
){
    // 先处理 3种 特殊情况, 保证 vec.size() >= 3
    if (vec.size() == 0) {
        vec.emplace_back(newData);
        return;
    } else if (newData.PerfIndex >= vec.front().PerfIndex) {
        vec.insert(vec.begin(), newData);
        return;
    } else if (newData.PerfIndex <= vec.back().PerfIndex) {
        vec.insert(vec.end(), newData);
        return;
    }

    int begin = 0, end = vec.size();
    while (end - begin >= 2) {
        int middle = (begin + end) / 2;
        if (newData.PerfIndex > vec[middle].PerfIndex) {
            end = middle;
        } else {
            begin = middle;
        }
    }

    auto tmpIter = vec.begin() + end;
    vec.insert(tmpIter, newData);
    return;
}

void CmpPerf_Insert(
    std::vector<std::pair<double, double>>& vec, 
    std::pair<double, double>& newData
){
    // 先处理 3种 特殊情况, 保证 vec.size() >= 3
    if (vec.size() == 0) {
        vec.emplace_back(newData);
        return;
    } else if (newData.first >= vec.front().first) {
        vec.insert(vec.begin(), newData);
        return;
    } else if (newData.first <= vec.back().first) {
        vec.insert(vec.end(), newData);
        return;
    }

    int begin = 0, end = vec.size();
    while (end - begin >= 2) {
        int middle = (begin + end) / 2;
        if (newData.first > vec[middle].first) {
            end = middle;
        } else {
            begin = middle;
        }
    }

    auto tmpIter = vec.begin() + end;
    vec.insert(tmpIter, newData);
    return;
}

// 在 vec 中找到 时间 稳定度指标刚好大于 TimeStableIndex 的项目, 在其之前插入 newData
template <class BASE_CTRL_PAIR>
void CmpStability_Insert(
    std::vector<BASE_CTRL_PAIR>& vec, 
    BASE_CTRL_PAIR& newData
){
    // 先处理 3种 特殊情况, 保证 vec.size() >= 3
    if (vec.size() == 0) {
        vec.emplace_back(newData);
        return;
    } else if (newData.StableIndex <= vec.front().StableIndex) {
        vec.insert(vec.begin(), newData);
        return;
    } else if (newData.StableIndex >= vec.back().StableIndex) {
        vec.insert(vec.end(), newData);
        return;
    }

    int begin = 0, end = vec.size();
    while (end - begin >= 2) {
        int middle = (begin + end) / 2;
        if (newData.StableIndex < vec[middle].StableIndex) {
            end = middle;
        } else {
            begin = middle;
        }
    }

    auto tmpIter = vec.begin() + end;
    vec.insert(tmpIter, newData);
    return;
}

// wfr 20220724 计算 两组测量数据中 同种 kernel 执行时间占比, 计算 复合执行时间指标
// 先计算同种 kernel, 即进行交集操作
// 再分别计算 两组测量数据 中 这些同种 kernel 执行时间占比
// 取较小的占比 返回
// 对聚类过的两个 kernel 数据序列, 需要改进算法, 匹配 gridSize 最近 且 满足 执行时间 波动约束
// 由于 聚类算法 的 修改, A 序列中 Ai 和 B 序列中多个 数据满足 gridSize, 需要找到 gridSize 最小距离 且 满足执行时间 波动约束 的 配置
// 返回 能在数据库中(BaseData)收录的(也考虑执行时间不稳定的kernel) 执行时间 占 CtrlData总执行时间 的百分比
// 目前数据库中 不再移除 执行时间不稳定的 kernel
double AnalysisKernelData(
    float& CoveredExeTimePct, float& MatchedExeTimePct, float& MatchedGapTimePct, 
    float& ExeIndex, float& GapIndex, unsigned int& MatchedStreams, float& UnMatchedStreamExeTimePct,
    bool isDiscount, MEASURE_DATA& BaseData, MEASURE_DATA& CtrlData, 
    float ExeTimeRadius, float GapTimeRadius, OPTIMIZATION_OBJECTIVE OptObj, double PerfLossConstraint
){

    std::cout << "AnalysisKernelData: 进入" << std::endl;

    double PerfIndex = 0.0;

    if (BaseData.map2Kernel.size() == 0) {
        PerfIndex = 0.05;
        CoveredExeTimePct = 0.0;
        return PerfIndex;
    }

    if (CtrlData.map2Kernel.size() == 0) {
        PerfIndex = 0.05;
        CoveredExeTimePct = 0.94;
        return PerfIndex;
    }

    CtrlData.CoveredExeTime = 0.0;
    CtrlData.CoveredGapTime = 0.0;
    CtrlData.UnCoveredExeTime = 0.0;
    CtrlData.UnCoveredGapTime = 0.0;

    // 想办法计算 在 Ctrl 的 stream 结构下, 但 在默认配置下的 性能
    // 减去 Ctrl 和 Base 的 执行时间的差值, 不可能每种 kernel 都能匹配上, 
    // 只能是 用匹配到的 kernel 的执行时间变化百分比 推出 没匹配到的 kernel 的执行时间变化

    // 对于一个 stream: MeasureDuration - stream中kernel数量 * kernel执行时间变化平均百分比
    // 在 stream 中的总执行时间中 扣除 平均的 执行时间变化

    std::map<int, int> mapNumExeCtrl;
    std::map<int, int> mapNumGapCtrl;

    std::map<int, double> mapMatchedExeTimeCtrl; // 不同 stream 中被匹配到的 ExeTime 累加
    std::map<int, double> mapMatchedGapTimeCtrl; // 不同 stream 中被匹配到的 GapTime 累加

    std::map<int, double> mapWeightedSumExeTimeBase;
    std::map<int, double> mapWeightedSumExeTimeCtrl;
    std::map<int, double> mapWeightedAvgExeTimeBase; // 平均执行时间
    std::map<int, double> mapWeightedAvgExeTimeCtrl; // 平均执行时间

    std::map<int, double> mapWeightedSumGapTimeBase;
    std::map<int, double> mapWeightedSumGapTimeCtrl;
    std::map<int, double> mapWeightedAvgGapTimeBase; // 平均 gap 时间
    std::map<int, double> mapWeightedAvgGapTimeCtrl; // 平均 gap 时间

    int NumExeCtrl = 0, NumGapCtrl = 0;
    double AccuExeTimeCtrl = 0.0;
    double WeightedSumExeTimeBase = 0.0, WeightedSumExeTimeCtrl = 0.0;
    double WeightedAvgExeTimeBase = 0.0, WeightedAvgExeTimeCtrl = 0.0;

    double AccuGapTimeCtrl = 0.0;
    double WeightedSumGapTimeBase = 0.0, WeightedSumGapTimeCtrl = 0.0;
    double WeightedAvgGapTimeBase = 0.0, WeightedAvgGapTimeCtrl = 0.0;

    float CoveredGapTimePct = 0.0;
    float UnCoveredExeTimePct = 0.0, UnCoveredGapTimePct = 0.0;

    // 匹配 Exe 时间
    for (auto& pairMapKernel : CtrlData.map2Kernel) {
        int StreamID = pairMapKernel.first;
        // 判断是否有相同 stream
        auto tmpIter = BaseData.map2Kernel.find(StreamID);
        if (tmpIter == BaseData.map2Kernel.end()) {
            // std::cout << "AnalysisKernelData: BaseData 没有 Stream " << StreamID << std::endl;
            // 累加没匹配到的 KernelTime
            for (const auto& pairKernel : pairMapKernel.second) {
                CtrlData.UnCoveredExeTime += pairKernel.second.sumExeTime;
            }
            continue; // 没有相同 stream 则 下次循环
        }

        mapNumExeCtrl[StreamID] = 0;
        mapMatchedExeTimeCtrl[StreamID] = 0.0;
        mapWeightedSumExeTimeBase[StreamID] = 0.0;
        mapWeightedSumExeTimeCtrl[StreamID] = 0.0;
        mapWeightedAvgExeTimeBase[StreamID] = 0.0;
        mapWeightedAvgExeTimeCtrl[StreamID] = 0.0;

        vecExeDataPair.clear();

        auto& mapKernelBase = BaseData.map2Kernel[StreamID];
        auto& mapKernelCtrl = CtrlData.map2Kernel[StreamID];

        // MFGPOEOOutStream << "AnalysisKernelData: Stream " << std::dec << StreamID << ": " << std::endl;
        for (auto& pairKernelCtrl : mapKernelCtrl) {
            std::string KernelID = pairKernelCtrl.first;

            auto tmpIter = mapKernelBase.find(KernelID);
            if (tmpIter == mapKernelBase.end() || mapKernelBase[KernelID].count == 0) {
                // std::cout << "AnalysisKernelData: BaseData Stream " << StreamID << " 没有 " << KernelID << std::endl;
                // std::cout << "AnalysisKernelData: BaseData Stream " << StreamID << KernelID << " 向量为空" << std::endl;
                // 累加没匹配到的 ExeTime
                CtrlData.UnCoveredExeTime += pairKernelCtrl.second.sumExeTime;
                continue; // 没有相同 kernel name 则 下次循环
            }
            if (mapKernelCtrl[KernelID].count == 0) {
                // std::cout << "AnalysisKernelData: CtrlData Stream " << StreamID << KernelID << " 向量为空" << std::endl;
                continue;
            }

            CtrlData.CoveredExeTime += pairKernelCtrl.second.sumExeTime;
            // 下边将数据插入到 vecExeDataPair 的适当位置, 保证 std 升序
            BASE_CTRL_EXE_PAIR tmpExeDataPair;

            tmpExeDataPair.StableIndex = std::max(mapKernelBase[KernelID].ExeTimeStableIndex, mapKernelCtrl[KernelID].ExeTimeStableIndex);
            tmpExeDataPair.pBase = &mapKernelBase[KernelID];
            tmpExeDataPair.pCtrl = &mapKernelCtrl[KernelID];
            tmpExeDataPair.PerfIndex = (tmpExeDataPair.pCtrl->avgExeTime / tmpExeDataPair.pBase->avgExeTime - 1.0);
            // 处理降频后性能反而异常提升的问题, 直接将 < 0.0 的 PerfIndex 置为 0.0
            // if (CtrlData.SMClkUpperLimit < BaseData.SMClkUpperLimit // 频率上限降低
            //     && BaseData.avgSMClk > CtrlData.avgSMClk // 实际频率降低
            //     && tmpExeDataPair.PerfIndex < GlobalConfig.StrangePerfThreshold * GlobalConfig.objective) // 性能提升 / 性能损失太小
            // {
            //     tmpExeDataPair.PerfIndex = 0.0;
            // }
            
            CmpStability_Insert(vecExeDataPair, tmpExeDataPair);
        }

        std::vector<BASE_CTRL_EXE_PAIR> vecMatchedExe;
        // MFGPOEOOutStream << "AnalysisKernelData: Stream " << std::dec << StreamID << ": " << std::endl;
        // MFGPOEOOutStream << "vecExeDataPair.size() = " << std::dec << vecExeDataPair.size() << std::endl;
        for (auto data : vecExeDataPair) {
            if (data.StableIndex >= LARGE_DOUBLE) { // 如果时间分布过于分散
                MFGPOEOOutStream << "ExeTime 分布过于分散, StableIndex = " << std::scientific << std::setprecision(4) << data.StableIndex << std::endl;
                break;
            }
            
            // 如果时间分布已经较为分散 就不再累加
            if (data.StableIndex > ExeTimeRadius) {
                
                // MFGPOEOOutStream << "ExeTime 跳出" << std::endl;
                break;
            }

            // 按 PerfIndex 降序排列
            CmpPerf_Insert(vecMatchedExe, data);

            float tmpPerfIndex = (data.pCtrl->avgExeTime / data.pBase->avgExeTime - 1.0);
            // 处理降频后性能反而异常提升的问题
            // if (CtrlData.SMClkUpperLimit < BaseData.SMClkUpperLimit // 频率上限降低
            //     && BaseData.avgSMClk > CtrlData.avgSMClk // 实际频率降低
            //     && tmpPerfIndex < GlobalConfig.StrangePerfThreshold * GlobalConfig.objective) // 性能提升 / 性能损失太小
            // {   // 如果性能异常提升 累加相同的 加权执行时间
            //     mapMatchedExeTimeCtrl[StreamID] += data.pCtrl->sumExeTime;
            // } else {
                mapNumExeCtrl[StreamID] += data.pCtrl->count;
                mapMatchedExeTimeCtrl[StreamID] += data.pCtrl->sumExeTime;
                mapWeightedSumExeTimeBase[StreamID] += data.pCtrl->count * data.pBase->avgExeTime;
                mapWeightedSumExeTimeCtrl[StreamID] += data.pCtrl->count * data.pCtrl->avgExeTime;
            // }
        }

        // 当前 steam 没 匹配到 kernel 则 删除相关项 并 下次循环
        if (mapNumExeCtrl[StreamID] == 0) {
            mapNumExeCtrl.erase(StreamID);
            mapMatchedExeTimeCtrl.erase(StreamID);
            mapWeightedSumExeTimeBase.erase(StreamID);
            mapWeightedSumExeTimeCtrl.erase(StreamID);
            mapWeightedAvgExeTimeBase.erase(StreamID);
            mapWeightedAvgExeTimeCtrl.erase(StreamID);
            continue;
        }

        NumExeCtrl += mapNumExeCtrl[StreamID]; // 按照 Ctrl 数据进行加权
        AccuExeTimeCtrl += mapMatchedExeTimeCtrl[StreamID];
        WeightedSumExeTimeBase += mapWeightedSumExeTimeBase[StreamID];
        WeightedSumExeTimeCtrl += mapWeightedSumExeTimeCtrl[StreamID];

        if (mapNumExeCtrl[StreamID] > 0) {
            mapWeightedAvgExeTimeBase[StreamID] = mapWeightedSumExeTimeBase[StreamID] / mapNumExeCtrl[StreamID];
            mapWeightedAvgExeTimeCtrl[StreamID] = mapWeightedSumExeTimeCtrl[StreamID] / mapNumExeCtrl[StreamID];
        }
    }

    // 匹配 Gap 时间
    for (auto& pairMapGap : CtrlData.map2Gap) {
        int StreamID = pairMapGap.first;

        // MFGPOEOOutStream << "AnalysisKernelData: Gap StreamID: " << std::dec << StreamID << std::endl;

        // 当前 steam 没 匹配到 kernel 则 下次循环
        auto iterNumExeCtrl = mapNumExeCtrl.find(StreamID);
        if (iterNumExeCtrl == mapNumExeCtrl.end()) {
            MFGPOEOOutStream << "AnalysisKernelData: Gap: Stream " << std::dec << StreamID << " 没有匹配到 Exe 时间" << std::endl;
            continue;
        }

        // 判断是否有相同 stream
        auto tmpIter = BaseData.map2Gap.find(StreamID);
        if (tmpIter == BaseData.map2Gap.end()) {
            // std::cout << "AnalysisKernelData: BaseData 没有 Stream " << StreamID << std::endl;
            // 累加没匹配到的 GapTime
            for (const auto& pairGap : pairMapGap.second) {
                CtrlData.UnCoveredGapTime += pairGap.second.sumGapTime;
            }
            continue; // 没有相同 stream 则 下次循环
        }

        mapNumGapCtrl[StreamID] = 0;
        mapMatchedGapTimeCtrl[StreamID] = 0.0;
        mapWeightedSumGapTimeBase[StreamID] = 0.0;
        mapWeightedSumGapTimeCtrl[StreamID] = 0.0;
        mapWeightedAvgGapTimeBase[StreamID] = 0.0;
        mapWeightedAvgGapTimeCtrl[StreamID] = 0.0;

        vecGapDataPair.clear();

        auto& mapGapBase = BaseData.map2Gap[StreamID];
        auto& mapGapCtrl = CtrlData.map2Gap[StreamID];

        for (auto& pairGapCtrl : mapGapCtrl) {
            std::string GapName = pairGapCtrl.first;

            auto tmpIter = mapGapBase.find(GapName);
            if (tmpIter == mapGapBase.end() || mapGapBase[GapName].count == 0) {
                // std::cout << "AnalysisKernelData: BaseData Stream " << StreamID << " 没有 " << KernelName << std::endl;
                // std::cout << "AnalysisKernelData: BaseData Stream " << StreamID << KernelName << " 向量为空" << std::endl;
                // 累加没匹配到的 GapTime
                CtrlData.UnCoveredGapTime += pairGapCtrl.second.sumGapTime;
                continue; // 没有相同 kernel name 则 下次循环
            }
            if (mapGapCtrl[GapName].count == 0) {
                // std::cout << "AnalysisKernelData: CtrlData Stream " << StreamID << KernelName << " 向量为空" << std::endl;
                continue;
            }

            CtrlData.CoveredGapTime += pairGapCtrl.second.sumGapTime;
            // 下边将数据插入到 vecGapDataPair 的适当位置, 保证 std 升序
            BASE_CTRL_GAP_PAIR tmpGapDataPair;

            tmpGapDataPair.StableIndex = std::max(mapGapBase[GapName].GapTimeStableIndex, mapGapCtrl[GapName].GapTimeStableIndex);
            
            tmpGapDataPair.pBase = &mapGapBase[GapName];
            tmpGapDataPair.pCtrl = &mapGapCtrl[GapName];
            tmpGapDataPair.PerfIndex = (tmpGapDataPair.pCtrl->avgGapTime / tmpGapDataPair.pBase->avgGapTime - 1.0);
            CmpStability_Insert(vecGapDataPair, tmpGapDataPair);

        }
    
        // MFGPOEOOutStream << "AnalysisKernelData: Stream " << std::dec << StreamID << ": " << std::endl;
        // MFGPOEOOutStream << "vecGapDataPair.size() = " << std::dec << vecGapDataPair.size() << std::endl;
        for (const auto data : vecGapDataPair) {
            if (data.StableIndex >= LARGE_DOUBLE) { // 如果时间分布过于分散
                MFGPOEOOutStream << "GapTime 分布过于分散, StableIndex = " << std::scientific << std::setprecision(4) << data.StableIndex << std::endl;
                break;
            }
            // 如果数量太少就不匹配
            if (data.pBase->count <= 1 || data.pCtrl->count <= 1)
            // if (data.pBase->count <= 0 || data.pCtrl->count <= 0)/
            {
                // MFGPOEOOutStream << "Gap 数量太少" << std::endl;
                continue;
            }

            // 之后, 如果时间分布已经较为分散 就不再累加
            if (data.StableIndex > GlobalConfig.GapTimeRadius) {
                // MFGPOEOOutStream << "GapTime 跳出: StableIndex = " << std::fixed << std::setprecision(4) << data.StableIndex << std::endl;
                break;
            }

            mapNumGapCtrl[StreamID] += data.pCtrl->count;
            mapMatchedGapTimeCtrl[StreamID] += data.pCtrl->sumGapTime;
            mapWeightedSumGapTimeBase[StreamID] += data.pCtrl->count * data.pBase->avgGapTime;
            mapWeightedSumGapTimeCtrl[StreamID] += data.pCtrl->count * data.pCtrl->avgGapTime;
        }
        // MFGPOEOOutStream << "MatchedGapTimePct = " << std::fixed << std::setprecision(2) << mapMatchedGapTimeCtrl[StreamID] / CtrlData.mapStream[StreamID].sumGapTime * 100 << " %" << std::endl;

        NumGapCtrl += mapNumGapCtrl[StreamID]; // 按照 Ctrl 数据进行加权
        AccuGapTimeCtrl += mapMatchedGapTimeCtrl[StreamID];
        WeightedSumGapTimeBase += mapWeightedSumGapTimeBase[StreamID];
        WeightedSumGapTimeCtrl += mapWeightedSumGapTimeCtrl[StreamID];

        if (mapNumGapCtrl[StreamID] > 0) {
            mapWeightedAvgGapTimeBase[StreamID] = mapWeightedSumGapTimeBase[StreamID] / mapNumGapCtrl[StreamID];
            mapWeightedAvgGapTimeCtrl[StreamID] = mapWeightedSumGapTimeCtrl[StreamID] / mapNumGapCtrl[StreamID];
        }
    }

    // 对 总体 数据求加权平均
    if (NumExeCtrl > 0) { // 如果匹配到了数据
        WeightedAvgExeTimeBase = WeightedSumExeTimeBase / NumExeCtrl;
        WeightedAvgExeTimeCtrl = WeightedSumExeTimeCtrl / NumExeCtrl;
    }

    if (NumGapCtrl > 0) {
        WeightedAvgGapTimeBase = WeightedSumGapTimeBase / NumGapCtrl;
        WeightedAvgGapTimeCtrl = WeightedSumGapTimeCtrl / NumGapCtrl;
    }
    
    // 匹配到的时间 占 累计时间的 比例
    MatchedExeTimePct = AccuExeTimeCtrl / (CtrlData.AccuExeTime + 1e-12);
    MatchedGapTimePct = AccuGapTimeCtrl / (CtrlData.AccuGapTime + 1e-12);
    // 覆盖的时间 占 累计时间的 比例
    CoveredExeTimePct = CtrlData.CoveredExeTime / (CtrlData.AccuExeTime + 1e-12);
    CoveredGapTimePct = CtrlData.CoveredGapTime / (CtrlData.AccuGapTime + 1e-12);

    UnCoveredExeTimePct = CtrlData.UnCoveredExeTime / (CtrlData.AccuExeTime + 1e-12);
    UnCoveredGapTimePct = CtrlData.UnCoveredGapTime / (CtrlData.AccuGapTime + 1e-12);


    // Ctrl 的总执行时间 (包括 Gap)
    double sumPerfIndex = 0.0, sumGapTimeInc = 0.0, sumExeIndex = 0.0, sumGapIndex = 0.0;
    double sumWeight = 0.0;
    double avgPerfIndex = 0.0;
    int tmpCount = 0;
    double avgGapWeight = 0.0;
    for (auto pairNumCtrl : mapNumExeCtrl) {
        int StreamID = pairNumCtrl.first;
        if (pairNumCtrl.second == 0) {
            continue;
        }
        tmpCount++;
        avgGapWeight += std::max(mapMatchedGapTimeCtrl[StreamID] / CtrlData.MeasureDuration, 1e-5);
    }
    avgGapWeight /= (double)tmpCount;
    // 计算性能指标
    tmpCount = 0;
    for (auto pairNumCtrl : mapNumExeCtrl) {
        int StreamID = pairNumCtrl.first;
        if (pairNumCtrl.second == 0) {
            continue;
        }
        tmpCount++;
        if (mapNumGapCtrl[StreamID] == 0)
        {
            continue;
        }        
        double tmpPerfIndex;
        double tmpTimeCtrl = CtrlData.MeasureDuration;
        double tmpTimeBase;
        double tmpMatchedExeTimePct = 0.0;
        double tmpMatchedGapTimePct = 0.0;
        tmpMatchedExeTimePct = mapMatchedExeTimeCtrl[StreamID] / CtrlData.mapStream[StreamID].sumExeTime;
        if (CtrlData.mapStream[StreamID].sumGapTime < 1e-9) {
            tmpMatchedGapTimePct = 0.0;
            MFGPOEOOutStream << "Stream " << std::dec << StreamID << ": CtrlData.mapStream[StreamID].sumGapTime = " << std::scientific << std::setprecision(4) << CtrlData.mapStream[StreamID].sumGapTime << " s" << std::endl;
        } else {
            tmpMatchedGapTimePct = mapMatchedGapTimeCtrl[StreamID] / CtrlData.mapStream[StreamID].sumGapTime;
        }
        
        // 计算 ExeTimeBase 部分
        double ExeTimeIncPct, GapTimeIncPct;
        double ExeBaseCtrlPct, GapBaseCtrlPct;
        ExeBaseCtrlPct = mapWeightedSumExeTimeBase[StreamID] / mapWeightedSumExeTimeCtrl[StreamID];
        ExeTimeIncPct = mapWeightedSumExeTimeCtrl[StreamID] / mapWeightedSumExeTimeBase[StreamID] - 1;
        // 按比例换算, Ctrl负载中 kernel部分 若在默认配置下的 时间
        double ExeTimeBase = CtrlData.mapStream[StreamID].sumExeTime * ExeBaseCtrlPct;

        GapBaseCtrlPct = 1.0;
        if (mapNumGapCtrl[StreamID] != 0) { // 如果匹配到了 Gap 就计算 比例
            GapBaseCtrlPct = mapWeightedSumGapTimeBase[StreamID] / mapWeightedSumGapTimeCtrl[StreamID];
        }
        GapTimeIncPct = 1 / GapBaseCtrlPct - 1;
        
        if (isDiscount == true && ExeTimeIncPct > -0.03 && GapTimeIncPct < 0.0)
        {
            MFGPOEOOutStream << std::fixed << std::setprecision(2) << "GapTimeIncPct: " << GapTimeIncPct * 100 << " %" << std::endl;
            
            // if (BaseData.avgMemUtil < 2.0)
            // {
            //     GapTimeIncPct *= 0.1;
            // } else 
            if (BaseData.avgMemUtil < 20.0 && BaseData.avgGPUUtil < 95.0)
            {
                if (CtrlData.mapStream[StreamID].sumGapTime / CtrlData.mapStream[StreamID].sumExeTime > 0.8)
                {
                    GapTimeIncPct *= 0.95;
                } else
                {
                    GapTimeIncPct *= 0.85;
                }
                
            } else if (BaseData.avgMemUtil < 50.0 && BaseData.avgGPUUtil < 95.0)
            {
                if (CtrlData.mapStream[StreamID].sumGapTime / CtrlData.mapStream[StreamID].sumExeTime > 0.8)
                {
                    GapTimeIncPct *= 0.75;
                } else
                {
                    GapTimeIncPct *= 0.83;
                }
            }

            GapBaseCtrlPct = 1/(GapTimeIncPct + 1);
            MFGPOEOOutStream << std::fixed << std::setprecision(2) << "GapTimeIncPct: " << GapTimeIncPct * 100 << " %" << std::endl;
        }

        // 计算 GapTimeBase 部分
        double GapTimeCtrl = 1.0;
        double GapTimeBase = 1.0;
        
        MFGPOEOOutStream << "Stream " << std::dec << StreamID << ": sumExeTime / MeasureDuration = " << std::fixed << std::setprecision(2) << BaseData.mapStream[StreamID].sumExeTime / BaseData.MeasureDuration * 100 << " %; " << "BaseData.avgGPUUtil = " << BaseData.avgGPUUtil << " %" << std::endl;
        

        // float unRecordedTime = CtrlData.MeasureDuration - CtrlData.mapStream[StreamID].sumExeTime - CtrlData.mapStream[StreamID].sumGapTime;
        float tmpDiscount;
        if (BaseData.avgMemUtil < 10)
        {
            tmpDiscount = 1.0;
        } else if (CtrlData.mapStream[StreamID].sumExeTime / CtrlData.mapStream[StreamID].sumGapTime > 5)
        {
            tmpDiscount = 0.45;
        } else {
            tmpDiscount = 0.65;
        }
        tmpDiscount = 1.0;

        // tmpTimeCtrl = CtrlData.mapStream[StreamID].sumExeTime + CtrlData.mapStream[StreamID].sumGapTime * tmpDiscount;
        // // tmpTimeCtrl = CtrlData.MeasureDuration;
        // GapTimeCtrl = tmpTimeCtrl - CtrlData.mapStream[StreamID].sumExeTime;
        // GapTimeBase = GapTimeCtrl * GapBaseCtrlPct;
        // // GapTimeBase = (GapTimeCtrl - mapMatchedGapTimeCtrl[StreamID]) + mapMatchedGapTimeCtrl[StreamID] * GapBaseCtrlPct;

        tmpTimeCtrl = CtrlData.mapStream[StreamID].sumExeTime + mapWeightedSumGapTimeCtrl[StreamID];
        GapTimeCtrl = mapWeightedSumGapTimeCtrl[StreamID];
        GapTimeBase = mapWeightedSumGapTimeBase[StreamID];


        tmpTimeBase = ExeTimeBase + GapTimeBase;
        tmpPerfIndex = tmpTimeCtrl / tmpTimeBase - 1;
        ExeIndex = ExeTimeIncPct;
        GapIndex = GapTimeCtrl / GapTimeBase - 1;

        // 累计加权性能指标
        // double tmpWeight = CtrlData.mapStream[StreamID].sumExeTime;
        // double tmpWeight = BaseData.mapStream[StreamID].sumExeTime;
        // double tmpWeight = std::sqrt(BaseData.mapStream[StreamID].sumExeTime);
        double tmpWeight = 1.0;
        
        tmpWeight = CtrlData.mapStream[StreamID].sumExeTime / CtrlData.MeasureDuration;
        tmpWeight = std::pow(tmpWeight, 2);
        sumWeight += tmpWeight;
        sumPerfIndex += tmpPerfIndex * tmpWeight;

        sumExeIndex += ExeIndex * tmpWeight;
        sumGapIndex += GapIndex * tmpWeight;

        std::pair<double, double> pairPerf(CtrlData.mapStream[StreamID].sumExeTime, tmpPerfIndex);

        MFGPOEOOutStream << "Stream " << std::dec << pairNumCtrl.first << ": KernelCount = " << CtrlData.mapStream[StreamID].KernelCount << "; Weight = " << std::scientific << std::setprecision(4) << tmpWeight << std::endl;

        MFGPOEOOutStream << "MatchedExeTimePct: " << std::fixed << std::setprecision(2) << tmpMatchedExeTimePct * 100 << " %; " << "MatchedGapTimePct: " << std::fixed << std::setprecision(2) << tmpMatchedGapTimePct * 100 << " %" << std::endl;

        MFGPOEOOutStream << std::scientific << std::setprecision(4) << "WeightedAvgExeTimeBase: " << mapWeightedAvgExeTimeBase[StreamID] << " s; WeightedAvgGapTimeBase: " << mapWeightedAvgGapTimeBase[StreamID] << " s" << std::endl;
        MFGPOEOOutStream << std::scientific << std::setprecision(4) << "WeightedAvgExeTimeCtrl: " << mapWeightedAvgExeTimeCtrl[StreamID] << " s; WeightedAvgGapTimeCtrl: " << mapWeightedAvgGapTimeCtrl[StreamID] << " s" << std::endl;
        
        MFGPOEOOutStream << std::scientific << std::setprecision(4) << "WeightedSumExeTimeBase: " << mapWeightedSumExeTimeBase[StreamID] << " s; WeightedSumGapTimeBase: " << mapWeightedSumGapTimeBase[StreamID] << " s" << std::endl;
        MFGPOEOOutStream << std::scientific << std::setprecision(4) << "WeightedSumExeTimeCtrl: " << mapWeightedSumExeTimeCtrl[StreamID] << " s; WeightedSumGapTimeCtrl: " << mapWeightedSumGapTimeCtrl[StreamID] << " s" << std::endl;

        MFGPOEOOutStream << std::fixed << std::setprecision(2) << "ExeTimeIncPct: " << ExeTimeIncPct * 100 << " %; GapTimeIncPct: " << GapTimeIncPct * 100 << " %" << std::endl;
        MFGPOEOOutStream << std::fixed << std::setprecision(4) << "sumExeTime: " << CtrlData.mapStream[StreamID].sumExeTime << " s; sumGapTime: " << CtrlData.mapStream[StreamID].sumGapTime << " s" << std::endl;
        MFGPOEOOutStream << "PerfIndex: " << std::fixed << std::setprecision(2) << tmpPerfIndex * 100 << " %" << std::endl;
    }
    avgPerfIndex = sumPerfIndex / (sumWeight + 1e-12);
    ExeIndex = sumExeIndex / (sumWeight + 1e-12);
    GapIndex = sumGapIndex / (sumWeight + 1e-12);
    MatchedStreams = tmpCount;

    // wfr 20230108 计算 CtrlData 中没有匹配到的 stream 的 ExeTime 占 CtrlData 总 ExeTime 的比例
    double UnMatchedStreamExeTime = 0.0;
    for (auto pairStream : CtrlData.mapStream)
    {
        int StreamID = pairStream.first;
        if (mapNumExeCtrl.count(StreamID) == 0 // mapNumExeCtrl 中没有 StreamID
            || mapNumExeCtrl[StreamID] == 0) // 或者没匹配到 kernel
        {
            UnMatchedStreamExeTime += CtrlData.mapStream[StreamID].sumExeTime;
        }
    }
    UnMatchedStreamExeTimePct = UnMatchedStreamExeTime / (CtrlData.AccuExeTime + 1e-12);

    // 出现 avgTimeBase 为 0 的情况
    if (tmpCount == 0) {
        MFGPOEOOutStream << "AnalysisKernelData: Error: avgPerfIndex = " << std::setprecision(4) << avgPerfIndex << " s; 可能没匹配到 kernel 数据" << std::endl;
        PerfIndex = 0.05;
    } else {
        PerfIndex = avgPerfIndex;
    }

    struct timeval tmpTimeStamp;
    gettimeofday(&tmpTimeStamp, NULL);
    double RelativeTimeStamp = (double)tmpTimeStamp.tv_sec + (double)tmpTimeStamp.tv_usec * 1e-6 - GlobalConfig.StartTimeStamp;
    MFGPOEOOutStream << std::endl;
    MFGPOEOOutStream << "Relative Time Stamp: " << std::fixed << std::setprecision(2) << RelativeTimeStamp << " s" << std::endl;
    
    MFGPOEOOutStream << "CoveredExeTimePct: " << std::fixed << std::setprecision(1) << 100 * CoveredExeTimePct << " %" << std::endl;
    MFGPOEOOutStream << "MatchedExeTimePct: " << std::fixed << std::setprecision(1) << 100 * MatchedExeTimePct << " %" << std::endl;
    MFGPOEOOutStream << "UnCoveredExeTimePct: " << std::fixed << std::setprecision(1) << 100 * UnCoveredExeTimePct << " %" << std::endl;
    
    MFGPOEOOutStream << "CoveredGapTimePct: " << std::fixed << std::setprecision(1) << 100 * CoveredGapTimePct << " %" << std::endl;
    MFGPOEOOutStream << "MatchedGapTimePct: " << std::fixed << std::setprecision(1) << 100 * MatchedGapTimePct << " %" << std::endl;
    MFGPOEOOutStream << "UnCoveredGapTimePct: " << std::fixed << std::setprecision(1) << 100 * UnCoveredGapTimePct << " %" << std::endl;

    MFGPOEOOutStream << "PerfIndex: " << std::fixed << std::setprecision(2) << PerfIndex * 100 << " %; " 
    << "ExeIndex: " << ExeIndex * 100 << " %; "
    << "GapIndex: " << GapIndex * 100 << " %" << std::endl;

    MFGPOEOOutStream << "UnMatchedStreamExeTimePct: " << std::fixed << std::setprecision(2) << 100 * UnMatchedStreamExeTimePct << " %" << std::endl;

    // 计算目标函数值
    double PwrRel = CtrlData.avgPower / BaseData.avgPower;
    if (OptObj == OPTIMIZATION_OBJECTIVE::ENERGY) {
        // PerfIndex 表示应用运行时间增长百分比, PerfIndex+1 即可表示归一化的应用运行时间, 再乘归一化的功率即可得到归一化的能耗
        CtrlData.Obj = PwrRel * (PerfIndex+1);
        // if (PerfLossConstraint >= 0.0 && PerfIndex > PerfLossConstraint)
        // {
        //     CtrlData.Obj += 1; // 性能损失超过阈值, 增大目标函数, 作为惩罚
        // }
    } else if (OptObj == OPTIMIZATION_OBJECTIVE::EDP) {
        CtrlData.Obj = PwrRel * ((PerfIndex+1)*(PerfIndex+1));
        // if (PerfLossConstraint >= 0.0 && PerfIndex > PerfLossConstraint)
        // {
        //     CtrlData.Obj += 1; // 性能损失超过阈值, 增大目标函数, 作为惩罚
        // }
    } else if (OptObj == OPTIMIZATION_OBJECTIVE::ED2P) {
        CtrlData.Obj = PwrRel * ((PerfIndex+1)*(PerfIndex+1)*(PerfIndex+1));
        // if (PerfLossConstraint >= 0.0 && PerfIndex > PerfLossConstraint)
        // {
        //     CtrlData.Obj += 1; // 性能损失超过阈值, 增大目标函数, 作为惩罚
        // }
    } else {
        CtrlData.Obj = PwrRel * (PerfIndex+1);
        // if (PerfLossConstraint >= 0.0 && PerfIndex > PerfLossConstraint)
        // {
        //     CtrlData.Obj += 1; // 性能损失超过阈值, 增大目标函数, 作为惩罚
        // }
    }
    CtrlData.ExeIndex = ExeIndex;
    CtrlData.GapIndex = GapIndex;
    CtrlData.PerfLoss = PerfIndex;
    
    std::cout << "AnalysisKernelData: done" << std::endl;

    return PerfIndex;
}
