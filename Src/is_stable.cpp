
/*******************************************************************************
Copyright(C), 2022-2022, 瑞雪轻飏
     FileName: is_stable.cpp
       Author: 瑞雪轻飏
      Version: 0.01
Creation Date: 20220615
  Description: 判断应用是否进入稳定阶段
       Others: 
*******************************************************************************/

#include "is_stable.h"
#include "global_var.h"
#include "measure.h"
#include "analysis.h"

// 返回以对应类型的key的vector
template < class T, class P >
std::vector<T> getKeys(typename std::map<T, P> in_map)
{
    std::vector<T> vecKey;
    for(auto iter = in_map.begin(); iter != in_map.end(); ++iter){
        vecKey.push_back(iter->first);
    }
    return vecKey;
}

unsigned int judgeConfigStability(unsigned int judgeLen, int NumGearsThreshold, int CtrlCount, double sumMeasurementTime, CIRCULAR_QUEUE<unsigned int>& queOptSMClkGear, double CtrlPctErrThreshold, CIRCULAR_QUEUE<double>& queCtrlPctErr, CIRCULAR_QUEUE<double>& queOptObj) {
    // queCtrlPctErr[i] > 0.0 超过 性能损失约约束
    // queCtrlPctErr[i] < 0.0 满足 性能损失约约束

    MFGPOEOOutStream << "judgeConfigStability: 进入" << std::endl;
    MFGPOEOOutStream << "judgeConfigStability: judgeLen = " << std::dec << judgeLen << std::endl;
    MFGPOEOOutStream << "judgeConfigStability: CtrlPctErrThreshold = " << std::dec << CtrlPctErrThreshold << std::endl;
    MFGPOEOOutStream << "judgeConfigStability: queOptSMClkGear.size = " << std::dec << queOptSMClkGear.size << std::endl;
    MFGPOEOOutStream << "judgeConfigStability: queCtrlPctErr.size = " << std::dec << queCtrlPctErr.size << std::endl;

    if (queOptSMClkGear.size - 1 < judgeLen || queCtrlPctErr.size < judgeLen) {
        return 0;
    }

    unsigned int minGear = queOptSMClkGear[-2];
    unsigned int maxGear = queOptSMClkGear[-2];
    unsigned int indexOpt = -1;
    double OptObj = queOptObj[indexOpt] + (queCtrlPctErr[indexOpt] > 0.0 ? 1.0 : 0.0);
    for (size_t i = -2; i >= -5; i--)
    {
        minGear = std::min(minGear, queOptSMClkGear[i-1]);
        maxGear = std::max(maxGear, queOptSMClkGear[i-1]);

        unsigned int tmpGear = queOptSMClkGear[i-1];
        unsigned int OptGear = queOptSMClkGear[indexOpt-1];
        double tmpErr = queCtrlPctErr[i];
        double OptErr = queCtrlPctErr[indexOpt];
        double tmpObj = queOptObj[i] + (queCtrlPctErr[i] > 0.0 ? 1.0 : 0.0);
        if (tmpObj < OptObj
            && ((tmpGear < OptGear && tmpErr > OptErr)||(tmpGear >= OptGear))
        ) {
            indexOpt = i;
            OptObj = tmpObj;
        }
    }
    
    unsigned int OptGear = queCtrlPctErr[indexOpt] > 0.0 ? maxGear : queOptSMClkGear[indexOpt-1];

    // 找到: 满足性能损失约束的档位, 且 高于此的档位都满足性能损失约束

    if ((sumMeasurementTime > 75 && CtrlCount >= 4) || CtrlCount >= 5)
    {
        MFGPOEOOutStream << "judgeConfigStability: 调整时间过长/次数过多, 跳出" << std::endl;
        return OptGear;
    }

    if (queOptSMClkGear[-1] == queOptSMClkGear[-2])
    {
        MFGPOEOOutStream << "judgeConfigStability: 连续相同档位, 跳出" << std::endl;
        // return queOptSMClkGear[-1];
        return OptGear;
    }

    if (maxGear - minGear <= 3 && queCtrlPctErr[-1] < 0.0)
    {
        MFGPOEOOutStream << "judgeConfigStability: 连续相近档位, 跳出" << std::endl;
        return OptGear;
    }

    // if (queOptSMClkGear[-1] >= 0.95 * vecSMClk.size())
    //     // && queOptSMClkGear[-2] >= vecSMClk.size() - 1
    //     // && queOptSMClkGear[-3] >= vecSMClk.size() - 1) 
    // {
    //     MFGPOEOOutStream << "judgeConfigStability: 接近核心频率上限, 跳出" << std::endl;
    //     return queOptSMClkGear[-1];
    // }
    
    unsigned int SMClkGear;
    
    // 这里可以允许一次异常值, 即任意排除一次的值, 如果满足所有条件也可以认为稳定
    double avgCtrlPctErr = 1e15;
    double avgAbsCtrlPctErr = 1e15;
    double sumCtrlPctErr = 0.0;
    double sumAbsCtrlPctErr = 0.0;
    int tmpEnd = -1 * judgeLen;
    double sumSMClkGear = 0.0;
    struct timeval tmpTimeStamp;

    // 如果收集的数据够多, 就多累加一个
    if (queOptSMClkGear.size - 1 > judgeLen && queCtrlPctErr.size > judgeLen) {
        --tmpEnd;
    }
    for (int i = -1; i >= tmpEnd; --i) {
        MFGPOEOOutStream << "judgeConfigStability: queOptSMClkGear[" << std::dec << i << "] = " << queOptSMClkGear[i-1] << std::endl;
        double tmpErr = queCtrlPctErr[i];
        MFGPOEOOutStream << "judgeConfigStability: queCtrlPctErr[" << std::dec << i << "] = " << std::fixed << std::setprecision(4) << tmpErr << std::endl;
        sumCtrlPctErr += tmpErr;
        sumAbsCtrlPctErr += std::abs(tmpErr);
    }

    // 尝试排除一个值, 即可以实现排除异常值的操作
    int indexExcluded = 0;
    if (tmpEnd + judgeLen == -1) {
        MFGPOEOOutStream << "judgeConfigStability: 尝试排除一个值" << std::endl;
        
        double tmpAvgPrev = (sumCtrlPctErr - queCtrlPctErr[-1]) / judgeLen;
        double tmpAvgAbsPrev = (sumAbsCtrlPctErr - std::abs(queCtrlPctErr[-1])) / judgeLen;

        for (int i = -1; i >= tmpEnd; --i) {
            // 计算排除一个后的平均误差
            double tmpErr = queCtrlPctErr[i];
            double tmpAvg = (sumCtrlPctErr - tmpErr) / judgeLen;
            double tmpAvgAbs = (sumAbsCtrlPctErr - std::abs(tmpErr)) / judgeLen;

            // 误差是负数 且 绝对值更小
            if (tmpAvg < 0 && tmpAvgAbs < avgAbsCtrlPctErr)
            {
                indexExcluded = i;
                avgCtrlPctErr = tmpAvg;
                avgAbsCtrlPctErr = tmpAvgAbs;
                MFGPOEOOutStream << "judgeConfigStability: indexExcluded = " << std::dec << indexExcluded << std::endl;
            }

            if (i == tmpEnd && indexExcluded == 0) { // 如果排除任意一个都不能满足约束, 就排除最旧的一个
                indexExcluded = tmpEnd;
                avgCtrlPctErr = tmpAvg;
                avgAbsCtrlPctErr = tmpAvgAbs;
                MFGPOEOOutStream << "judgeConfigStability: indexExcluded = " << std::dec << indexExcluded << std::endl;
                MFGPOEOOutStream << "judgeConfigStability: 排除最旧的一个" << std::endl;
            }
            
        }
    } else {
        avgCtrlPctErr = sumCtrlPctErr / std::abs(tmpEnd);
        avgAbsCtrlPctErr = sumAbsCtrlPctErr / std::abs(tmpEnd);
    }

    // wfr 20230103 低频率判断: 如果多次给出较低频率 且 没有到性能损失阈值, 此时可能再降频也不会超过性能损失阈值, 直接判断稳定
    bool flagLowSMClkGear = false;
    sumSMClkGear = 0.0;
    int tmpLen = 3;
    for (size_t i = -1; i >= -1 * tmpLen; --i)
    {
        if (queOptSMClkGear[i] < 0.6 * vecSMClk.size() && queCtrlPctErr[i] < 0.0)
        {
            flagLowSMClkGear = true;
            sumSMClkGear += queOptSMClkGear[i];
        } else
        {
            flagLowSMClkGear = false;
            break;
        }
    }
    if (flagLowSMClkGear == true && queCtrlPctErr[-1] <= 0.0)
    {
        SMClkGear = std::round(sumSMClkGear / tmpLen);
        MFGPOEOOutStream << "judgeConfigStability: 频率配置已经较低, 跳出" << std::endl;
        gettimeofday(&tmpTimeStamp, NULL);
        double RelativeTimeStamp = (double)tmpTimeStamp.tv_sec + (double)tmpTimeStamp.tv_usec * 1e-6 - GlobalConfig.StartTimeStamp;
        MFGPOEOOutStream << std::endl;
        MFGPOEOOutStream << "Relative Time Stamp: " << std::fixed << std::setprecision(2) << RelativeTimeStamp << std::endl;
        MFGPOEOOutStream << "avgSMClkGear: " << std::dec << SMClkGear << std::endl;
        MFGPOEOOutStream << std::endl;
        return SMClkGear;
    }

    if (avgAbsCtrlPctErr > CtrlPctErrThreshold) {
        SMClkGear = 0;
        MFGPOEOOutStream << "judgeConfigStability: 平均控制误差较大, avgAbsCtrlPctErr = " << std::fixed << std::setprecision(4) << avgAbsCtrlPctErr << std::endl;
        return SMClkGear;
    } else {
        MFGPOEOOutStream << "judgeConfigStability: avgAbsCtrlPctErr = " << std::fixed << std::setprecision(4) << avgAbsCtrlPctErr << std::endl;
    }
    if (avgCtrlPctErr > 0.0) {
        SMClkGear = 0;
        MFGPOEOOutStream << "judgeConfigStability: 平均性能损失超过阈值, avgCtrlPctErr = " << std::fixed << std::setprecision(4) << avgCtrlPctErr << std::endl;
        return SMClkGear;
    } else {
        MFGPOEOOutStream << "judgeConfigStability: avgCtrlPctErr = " << std::fixed << std::setprecision(4) << avgCtrlPctErr << std::endl;
    }
    if (queCtrlPctErr[-1] > 0.05) { // 如果 超过性能损失约束 5% 以上
        SMClkGear = 0;
        MFGPOEOOutStream << "judgeConfigStability: 最近配置性能损失超过阈值, CtrlPctErr = " << std::fixed << std::setprecision(4) << queCtrlPctErr[-1] << std::endl;
        return SMClkGear;
    }

    unsigned int minSMClkGear = 0xFFFFFFFF;
    unsigned int maxSMClkGear = 0;
    for (int i = -1; i >= tmpEnd; --i) {
        if (i == indexExcluded) continue;
        unsigned int tmpSMClk = queOptSMClkGear[i-1];
        // sumSMClkGear += tmpSMClk;
        minSMClkGear = std::min(tmpSMClk, minSMClkGear);
        maxSMClkGear = std::max(tmpSMClk, maxSMClkGear);
    }

    // 求 加权最优配置, 误差小权值大, 反之权值小, 只要满足性能约束的参与计算
    // avgAbsCtrlPctErr
    double sumWeight = 0.0;
    sumSMClkGear = 0.0;
    for (int i = -1; i >= tmpEnd; --i) {
        // if (i == indexExcluded || queCtrlPctErr[i] > 0.0) {
        //     continue;
        // }
        // double tmpWeight = 1 / (-1 * queCtrlPctErr[i] + avgAbsCtrlPctErr);

        if (i == indexExcluded) {
            continue;
        }
        double tmpWeight = 1.0 / std::abs(queCtrlPctErr[i]);
        tmpWeight = std::sqrt(tmpWeight);
        if (queCtrlPctErr[i] > 0.0) {
           tmpWeight /= 4.0; // 降低性能损失超过阈值的配置的权重
        }
        
        sumWeight += tmpWeight;
        sumSMClkGear += tmpWeight * queOptSMClkGear[i-1];
    }
    
    // 求 平均最优配置
    SMClkGear = std::round(sumSMClkGear / sumWeight);
    // if (queCtrlPctErr[-1] > 0.0)
    // {
    //     SMClkGear = std::max((unsigned int)queOptSMClkGear[-1], (unsigned int)SMClkGear);
    // }

    if (maxSMClkGear-minSMClkGear > NumGearsThreshold) {
        MFGPOEOOutStream << "judgeConfigStability: 频率配置波动较大; minSMClkGear = " << std::dec << minSMClkGear << "; maxSMClkGear = " << maxSMClkGear << std::endl;
        SMClkGear = 0;
    }

    gettimeofday(&tmpTimeStamp, NULL);
    double RelativeTimeStamp = (double)tmpTimeStamp.tv_sec + (double)tmpTimeStamp.tv_usec * 1e-6 - GlobalConfig.StartTimeStamp;
    MFGPOEOOutStream << std::endl;
    MFGPOEOOutStream << "Relative Time Stamp: " << std::fixed << std::setprecision(2) << RelativeTimeStamp << std::endl;
    MFGPOEOOutStream << "Config Distance: " << maxSMClkGear - minSMClkGear << std::endl;
    MFGPOEOOutStream << "avgSMClkGear: " << std::dec << SMClkGear << std::endl;
    MFGPOEOOutStream << std::endl;

    return SMClkGear;
}

// 计算两组 NVML EV 数据之间的百分比距离
float NVMLDistance(MEASURE_DATA& BaseData,MEASURE_DATA& Data) {

    double norm = 0.0;
    norm += (double)(BaseData.avgPower + Data.avgPower) / GlobalConfig.DefaultPowerLimit;
    norm += (double)(BaseData.avgSMClk + Data.avgSMClk) / vecSMClk.back();
    norm += (double)(BaseData.avgMemClk + Data.avgMemClk) / vecMemClk.back();
    norm += (double)(BaseData.avgGPUUtil + Data.avgGPUUtil) / 100;
    norm += (double)(BaseData.avgMemUtil + Data.avgMemUtil) / 100;

    double distance = 0.0;
    distance += abs(BaseData.avgPower - Data.avgPower) / GlobalConfig.DefaultPowerLimit;
    distance += abs(BaseData.avgSMClk - Data.avgSMClk) / vecSMClk.back();
    distance += abs(BaseData.avgMemClk - Data.avgMemClk) / vecMemClk.back();
    distance += abs(BaseData.avgGPUUtil - Data.avgGPUUtil) / 100;
    distance += abs(BaseData.avgMemUtil - Data.avgMemUtil) / 100;

    distance /= norm;

    struct timeval tmpTimeStamp;
    gettimeofday(&tmpTimeStamp, NULL);
    double RelativeTimeStamp = (double)tmpTimeStamp.tv_sec + (double)tmpTimeStamp.tv_usec * 1e-6 - GlobalConfig.StartTimeStamp;
    MFGPOEOOutStream << std::endl;
    MFGPOEOOutStream << "Relative Time Stamp: " << std::fixed << std::setprecision(2) << RelativeTimeStamp << std::endl;
    MFGPOEOOutStream 
    << "avgPower: " << std::fixed << std::setprecision(1) << Data.avgPower << "; " 
    << "avgSMClk: " << std::fixed << std::setprecision(1) << Data.avgSMClk << "; " 
    << "avgMemClk: " << std::fixed << std::setprecision(1) << Data.avgMemClk << "; " 
    << "avgGPUUtil: " << std::fixed << std::setprecision(1) << Data.avgGPUUtil << "; " 
    << "avgMemUtil: " << std::fixed << std::setprecision(1) << Data.avgMemUtil
    << std::endl;
    MFGPOEOOutStream << "NVML EV Distance: " << std::scientific << std::setprecision(4) << distance << std::endl;
    MFGPOEOOutStream << std::endl;

    return distance;
}


