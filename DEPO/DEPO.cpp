/*******************************************************************************
Copyright(C), 2022-2023, 瑞雪轻飏
     FileName: DEPO.cpp
       Author: 瑞雪轻飏
      Version: 0.01
Creation Date: 20230412
  Description: DEPO 主文件 (Dynamic GPU power capping with online performance tracing for energy efficient GPU computing using DEPO tool)
       Others: 
*******************************************************************************/

#include "DEPO.h"

void *EvaluatePerformance(void *arg)
{
    std::cout << "EvaluatePerformance 进入" << std::endl;
    CHECK_TERMINATION_RETURN NULL; // 如果停止标志为 true 直接跳出

    // 初始化: 
    float MeasureWindowDuration = DEPOCfg.MeasureWindowDuration; // 测量窗口长度 (s)
    struct timeval TimeStampBegin, TimeStampEnd;

    RawData.init(2); // 初始化测量缓冲区

//     // 修改频率配置 (频率配置从环境变量读取)
//     if (TestedSMClk < DEPOCfg.MaxSMClk)
//     {
//         NVMLConfig.SetSMClkUpperLimit(TestedSMClk);
//     }
//     DEPOOutStream << "EvaluatePerformance: TestedSMClk: " << std::dec << TestedSMClk << std::endl;
//     std::cout << "EvaluatePerformance: TestedSMClk: " << std::dec << TestedSMClk << std::endl;

// #if defined(SUPPORT_SET_MEMORY_CLOCK)
//     if (TestedMemClk < DEPOCfg.MaxMemClk)
//     {
//         NVMLConfig.SetMemClkUpperLimit(TestedMemClk);
//     }
//     DEPOOutStream << "EvaluatePerformance: TestedMemClk: " << std::dec << TestedMemClk << std::endl;
//     std::cout << "EvaluatePerformance: TestedMemClk: " << std::dec << TestedMemClk << std::endl;
// #endif

    if (TestedPwrLmt < DEPOCfg.DefaultPowerLimit)
    {
        NVMLConfig.SetPowerLimit(TestedPwrLmt);
    }
    DEPOOutStream << "EvaluatePerformance: TestedPwrLmt: " << std::dec << TestedPwrLmt << std::endl;
    std::cout << "EvaluatePerformance: TestedPwrLmt: " << std::dec << TestedPwrLmt << std::endl;

    sleep(16);

    // 启动测量
    // One subscriber is used to register multiple callback domains
    CUpti_SubscriberHandle subscriber;
    CUPTI_API_CALL(cuptiSubscribe(&subscriber, (CUpti_CallbackFunc)callbackHandler, NULL));
    // Runtime callback domain is needed for kernel launch callbacks
    CUPTI_API_CALL(cuptiEnableCallback(1, subscriber, CUPTI_CB_DOMAIN_DRIVER_API, CUPTI_DRIVER_TRACE_CBID_cuLaunchKernel));
    // Resource callback domain is needed for context creation callbacks
    CUPTI_API_CALL(cuptiEnableCallback(1, subscriber, CUPTI_CB_DOMAIN_RESOURCE, CUPTI_CBID_RESOURCE_CONTEXT_CREATED));

    int BaseMeasureCount = 0;
    // 测量基准数据
    BaseData.clear();
    while (!DEPOCfg.terminateThread && BaseMeasureCount < 4)
    {
        BaseMeasureCount++;

        measureBaseData(BaseData, MeasureWindowDuration, -1);

        CHECK_TERMINATION_BREAK; // 如果停止标志为 true 直接跳出

        DEPOOutStream << std::dec << std::fixed << std::setprecision(2) << "BaseData: " 
        << "KnlPerSec = " << BaseData.KnlPerSec << " k/s; "
        << "NumKnls = " << BaseData.NumKnls << " k; "
        << "T = " << BaseData.T << " s; "
        << "avgPwr = " << BaseData.avgPwr << " W" << endl;
    }
    CHECK_TERMINATION_RETURN NULL; // 如果停止标志为 true 直接跳出

    // 修改频率配置 (频率配置从环境变量读取)
//     if (TestedSMClk < DEPOCfg.MaxSMClk)
//     {
//         NVMLConfig.SetSMClkUpperLimit(TestedSMClk);
//     }
//     DEPOOutStream << "EvaluatePerformance: TestedSMClk: " << std::dec << TestedSMClk << std::endl;
//     std::cout << "EvaluatePerformance: TestedSMClk: " << std::dec << TestedSMClk << std::endl;

// #if defined(SUPPORT_SET_MEMORY_CLOCK)
//     if (TestedMemClk < DEPOCfg.MaxMemClk)
//     {
//         NVMLConfig.SetMemClkUpperLimit(TestedMemClk);
//     }
//     DEPOOutStream << "EvaluatePerformance: TestedMemClk: " << std::dec << TestedMemClk << std::endl;
//     std::cout << "EvaluatePerformance: TestedMemClk: " << std::dec << TestedMemClk << std::endl;
// #endif

    if (TestedPwrLmt < DEPOCfg.DefaultPowerLimit)
    {
        NVMLConfig.SetPowerLimit(TestedPwrLmt);
    }
    DEPOOutStream << "EvaluatePerformance: TestedPwrLmt: " << std::dec << TestedPwrLmt << std::endl;
    std::cout << "EvaluatePerformance: TestedPwrLmt: " << std::dec << TestedPwrLmt << std::endl;

    sleep(3); // 延时使GPU工作稳定

    // 进行测量
    MEASURED_DATA MeasuredData;
    measureData(MeasuredData, MeasureWindowDuration, 0.0);
    CHECK_TERMINATION_RETURN NULL; // 如果停止标志为 true 直接跳出

    // 计算 优化目标函数值 和 性能损失
    ObjFunc(MeasuredData, BaseData, DEPOCfg.OptObj, DEPOCfg.PerfLossConstraint);
    DEPOOutStream << std::dec << std::fixed << std::setprecision(2) << "DEPO EvaluatePerformance: " 
    << "MemClkLmt = " << TestedMemClk << " MHz; "
    << "SMClkLmt = " << TestedSMClk << " MHz; "
    << "PwrLmt = " << TestedPwrLmt << " W; "
    << "PerfLoss = " << MeasuredData.PerfLoss * 100 << " %; "
    << "Obj = " << MeasuredData.Obj << "; "
    << "KnlPerSec = " << MeasuredData.KnlPerSec << " k/s; "
    << "NumKnls = " << MeasuredData.NumKnls << " k; "
    << "T = " << MeasuredData.T << " s; "
    << "avgPwr = " << MeasuredData.avgPwr << " W" << endl;

    printTimeStamp();
    std::string Description = "PerformanceLoss = " + std::to_string(MeasuredData.PerfLoss);
    TimeStamp(Description);
    DEPOOutStream << "EvaluatePerformance: 性能损失 = " << std::fixed << std::setprecision(4)  << MeasuredData.PerfLoss * 100 << " %" << std::endl << std::flush;

    // wfr 20230418 在线性能评估完成后重置频率, 以更快完成之后的负载, 尽快结束实验
    NVMLConfig.ResetPowerLimit();
    NVMLConfig.ResetSMClkLimit();
    NVMLConfig.ResetMemClkLimit();

    // 结束测量
    // Detach CUPTI calling cuptiFinalize() API
    std::cout << "cuptiFinalize()" << std::endl;
    CUPTI_API_CALL(cuptiFinalize());

    std::cout << "EvaluatePerformance 结束" << std::endl;
    return NULL;
}

// DEPO 方法主函数
void *DEPOMain(void *arg)
{
    std::cout << "DEPO 进入" << std::endl;
    CHECK_TERMINATION_RETURN NULL; // 如果停止标志为 true 直接跳出

    int SkippedTime = 16;
    sleep(SkippedTime); // 延时一段时间 避开一开始的不稳定阶段

    // 初始化: 
    float MeasureWindowDuration = DEPOCfg.MeasureWindowDuration; // 测量窗口长度 (s)
    struct timeval TimeStampBegin, TimeStampEnd;

    RawData.init(2); // 初始化测量缓冲区

    // 启动测量
    // One subscriber is used to register multiple callback domains
    CUpti_SubscriberHandle subscriber;
    CUPTI_API_CALL(cuptiSubscribe(&subscriber, (CUpti_CallbackFunc)callbackHandler, NULL));
    // Runtime callback domain is needed for kernel launch callbacks
    CUPTI_API_CALL(cuptiEnableCallback(1, subscriber, CUPTI_CB_DOMAIN_DRIVER_API, CUPTI_DRIVER_TRACE_CBID_cuLaunchKernel));
    // Resource callback domain is needed for context creation callbacks
    CUPTI_API_CALL(cuptiEnableCallback(1, subscriber, CUPTI_CB_DOMAIN_RESOURCE, CUPTI_CBID_RESOURCE_CONTEXT_CREATED));

    int BaseMeasureCount = 0;
    // 测量基准数据
    BaseData.clear();
    while (!DEPOCfg.terminateThread && BaseMeasureCount < 4)
    {
        BaseMeasureCount++;

        measureBaseData(BaseData, MeasureWindowDuration, -1);

        CHECK_TERMINATION_BREAK; // 如果停止标志为 true 直接跳出

        DEPOOutStream << std::dec << std::fixed << std::setprecision(2) << "BaseData: " 
        << "KnlPerSec = " << BaseData.KnlPerSec << " k/s; "
        << "NumKnls = " << BaseData.NumKnls << " k; "
        << "T = " << BaseData.T << " s; "
        << "avgPwr = " << BaseData.avgPwr << " W" << endl;
    }
    CHECK_TERMINATION_RETURN NULL; // 如果停止标志为 true 直接返回

    // 下边进行搜索, 找到最优配置
    OptPwrGear = std::min((int)std::ceil(0.95 * vecPwrLmt.size()), (int)vecPwrLmt.size()-1);
    OptSMClkGear = std::min((int)std::ceil(0.95 * vecSMClk.size()), (int)vecSMClk.size()-1);
    OptMemClkGear = std::min((int)std::ceil(0.95 * vecMemClk.size()), (int)vecMemClk.size()-1);

    // int currPwrGear = std::min((int)std::ceil(0.95 * vecPwrLmt.size()), (int)vecPwrLmt.size()-1);
    // int currSMClkGear = std::min((int)std::ceil(0.95 * vecSMClk.size()), (int)vecSMClk.size()-1);
    // int currMemClkGear = std::min((int)std::ceil(0.95 * vecMemClk.size()), (int)vecMemClk.size()-1);

    // 搜索并设置最优配置
    OptPwrGear = GoldenSectionSearch(vecPwrLmt, GEAR_TYPE::PWR_LMT, 5);

    // // 搜索并设置最优配置
    // OptMemClkGear = GoldenSectionSearch(vecMemClk, GEAR_TYPE::MEM_CLK_LMT, 2);

    // // 搜索并设置最优配置
    // OptSMClkGear = GoldenSectionSearch(vecSMClk, GEAR_TYPE::SM_CLK_LMT, 5);

    // 结束测量
    // Detach CUPTI calling cuptiFinalize() API
    std::cout << "cuptiFinalize()" << std::endl;
    CUPTI_API_CALL(cuptiFinalize());

    std::cout << "DEPO 结束" << std::endl;
    return NULL;
}

int GoldenSectionSearch(vector<int>& vecGear, GEAR_TYPE GearType, int Error){

    string strGearType;
    string strUnit;
    if (GearType == GEAR_TYPE::PWR_LMT)
    {
        strGearType = "PwrLmt";
        strUnit = " W";
    } else if (GearType == GEAR_TYPE::SM_CLK_LMT)
    {
        strGearType = "SMClkLmt";
        strUnit = " MHz";
    } else
    {
        strGearType = "MemClkLmt";
        strUnit = " MHz";
    }
    DEPOOutStream << "\nGoldenSectionSearch: " << strGearType << endl << endl;

    // 计算默认(最高)配置下的 优化目标函数值 和 性能损失
    vecMeasuredData.clear();
    vecMeasuredData.resize(vecGear.size());
    vecMeasuredData.back() = BaseData;
    // 计算 优化目标函数值 和 性能损失
    vecMeasuredData.back().Gear = vecGear.size() - 1;
    ObjFunc(vecMeasuredData.back(), BaseData, DEPOCfg.OptObj, DEPOCfg.PerfLossConstraint);
    DEPOOutStream << std::dec << std::fixed << std::setprecision(2) << "DEPO: " 
    << strGearType << " = " << vecGear[vecMeasuredData.back().Gear] << strUnit << "; "
    << "PerfLoss = " << vecMeasuredData.back().PerfLoss * 100 << " %; "
    << "Obj = " << vecMeasuredData.back().Obj << "; "
    << "KnlPerSec = " << vecMeasuredData.back().KnlPerSec << " k/s; "
    << "NumKnls = " << vecMeasuredData.back().NumKnls << " k; "
    << "T = " << vecMeasuredData.back().T << " s; "
    << "avgPwr = " << vecMeasuredData.back().avgPwr << " W" << endl;


    float MeasureWindowDuration = DEPOCfg.MeasureWindowDuration; // 测量窗口长度 (s)
    int currGear = -2, GearLow = -1, GearGolden = -1, GearHigh = vecGear.size() - 1;

    float InitStep = 0.1;
    if (GearType == GEAR_TYPE::PWR_LMT)
    {
        // 找到默认情况下的档位, 以辅助黄金分割搜索的初始化
        float minDistance = vecGear.back();
        int defaultGear;
        for (int i = vecGear.size() - 1; i >= 0; i--)
        {
            float tmpDistance = std::abs((float)vecGear[i] - (float)BaseData.avgPwr);
            if (tmpDistance < minDistance)
            {
                minDistance = tmpDistance;
                defaultGear = i;
            } else
            {
                break;
            }
        }
        float tmpStep = (float)(vecGear.size() - defaultGear) / vecGear.size() * 0.85;
        InitStep = std::max(InitStep, tmpStep);
        DEPOOutStream << "GoldenSectionSearch: defaultGear = " << std::dec << defaultGear << endl;
    }
    DEPOOutStream << "GoldenSectionSearch: InitStep = " << std::fixed << std::setprecision(2) << InitStep << endl;

    // 先确定这三个档位, 再进行黄金分割搜索

    int GearGoldenMeasuredCount = 0;
    int OptGear = vecGear.size() - 1;
    while (!DEPOCfg.terminateThread)
    {
        if (GearGolden == currGear)
        {
            GearGoldenMeasuredCount++;
        }
        
        // 先确定 GearLow 和 GearGolden 初值, 保证 f_obj(GearLow) > f_obj(GearGolden)
        if (GearLow < 0 && GearGolden < 0) // 初始化 GearGolden
        {
            currGear = std::floor((float)GearHigh - InitStep * vecGear.size());
            GearGolden = currGear;
            GearGoldenMeasuredCount = 0;
        } else if (GearLow < 0 && GearGolden >= 0) // 初始化 GearLow
        {
            if (GearGoldenMeasuredCount <= 2
                && vecMeasuredData[GearGolden].PerfLoss > DEPOCfg.PerfLossConstraint) // 性能损失超过约束
            {
                GearLow = -1;
                currGear = std::floor((float)GearGolden + PHI*(GearHigh-GearGolden));
                GearGolden = currGear;
                GearGoldenMeasuredCount = 0;
                DEPOOutStream << "GearGolden 违反性能损失约束" << endl;
            } else if (currGear < GearGolden && vecMeasuredData[currGear].PerfLoss > DEPOCfg.PerfLossConstraint)
            {
                // GearLow = -1;
                // currGear = std::floor((float)currGear + RESPHI*(GearHigh-GearGolden));
                // if (currGear >= GearGolden)
                // {
                //     GearGolden = currGear;
                // }
                GearLow = currGear;
                currGear = -1;
                DEPOOutStream << "currGear 违反性能损失约束" << endl;
            } else if (currGear < GearGolden 
                && vecMeasuredData[currGear].Obj >= vecMeasuredData[GearGolden].Obj)
            {
                GearLow = currGear;
                currGear = -1;
                DEPOOutStream << "f_Obj(currGear) >= f_Obj(GearGolden)" << endl;
            } else if (currGear < GearGolden && vecMeasuredData[currGear].Obj < vecMeasuredData[GearGolden].Obj)
            {
                GearHigh = GearGolden;
                GearGolden = currGear;
                currGear = std::floor((float)GearGolden - 0.15*(GearHigh-GearGolden));
                currGear = std::min(currGear, GearGolden-1);
                DEPOOutStream << "f_Obj(currGear) < f_Obj(GearGolden)" << endl;
            } else if (GearGoldenMeasuredCount > 2 
                && currGear == GearGolden && GearGolden == GearHigh - 1 
                && vecMeasuredData[currGear].NumKnls > 0)
            {
                if (vecMeasuredData[GearGolden].PerfLoss > DEPOCfg.PerfLossConstraint
                    || vecMeasuredData[GearHigh].PerfLoss > DEPOCfg.PerfLossConstraint)
                {
                    OptGear = GearHigh;
                } else if (vecMeasuredData[GearGolden].Obj < vecMeasuredData[GearHigh].Obj)
                {
                    OptGear = GearGolden;
                } else
                {
                    OptGear = GearHigh;
                }
                DEPOOutStream << "满足性能损失约束的档位过少" << endl;
                break;
            } else
            {
                GearGolden = currGear;
                currGear = std::floor((float)GearGolden - 0.15*(GearHigh-GearGolden));
                currGear = std::min(currGear, GearGolden-1);
                DEPOOutStream << "currGear == GearGolden" << endl;
            }
            
        }
        
        // 开始黄金分割搜索
        if (0 <= GearLow && GearLow < GearGolden)
        {
            if (currGear < 0) // 如果没有测量数据
            {
                if (GearGolden >= (GearLow+GearHigh)/2) // 给出下一次的配置档位
                {
                    // currGear = std::round(GearLow + RESPHI * (GearHigh-GearLow));
                    currGear = std::round(GearGolden - RESPHI * (GearGolden-GearLow));
                } else if (GearGolden < (GearLow+GearHigh)/2)// 给出下一次的配置档位
                {
                    // currGear = std::round(GearLow + PHI * (GearHigh-GearLow));
                    currGear = std::round(GearGolden + RESPHI * (GearHigh-GearGolden));
                }
            } else if (GearGolden < currGear && vecMeasuredData[GearGolden].Obj <= vecMeasuredData[currGear].Obj)
            { // GearLow高 GearGolden低 currGear中  GearHigh高
                GearHigh = currGear;
                // currGear = std::round(GearLow + RESPHI*(GearHigh-GearLow));
                currGear = std::round(GearLow + PHI*(GearGolden-GearLow));
            }
            else if (GearGolden < currGear && vecMeasuredData[GearGolden].Obj > vecMeasuredData[currGear].Obj)
            { // GearLow高 GearGolden中 currGear低  GearHigh高
                GearLow = GearGolden;
                GearGolden = currGear;
                // currGear = std::round(GearLow + PHI*(GearHigh-GearLow));
                currGear = std::round(GearGolden + RESPHI*(GearHigh-GearGolden));

            } else if (currGear < GearGolden && vecMeasuredData[currGear].Obj <= vecMeasuredData[GearGolden].Obj)
            { // GearLow高 currGear低 GearGolden中 GearHigh高
                GearHigh = GearGolden;
                GearGolden = currGear;
                // currGear = std::round(GearLow + RESPHI*(GearHigh-GearLow));
                currGear = std::round(GearLow + PHI*(GearGolden-GearLow));

            } else if (currGear < GearGolden && vecMeasuredData[currGear].Obj > vecMeasuredData[GearGolden].Obj)
            { // GearLow高 currGear中 GearGolden低 GearHigh高
                GearLow = currGear;
                // currGear = std::round(GearLow + PHI*(GearHigh-GearLow));
                currGear = std::round(GearGolden + RESPHI*(GearHigh-GearGolden));
            }
        }

        currGear = std::max(currGear, 0);
        currGear = std::min(currGear, (int)vecGear.size()-1);
        DEPOOutStream << "currGear = " << vecGear[currGear] << strUnit << endl;

        if (GearLow >= 0)
        {
            DEPOOutStream << "Low = " << vecGear[GearLow] << strUnit;
        } else
        {
            DEPOOutStream << "Low = NaN" << strUnit;
        }
        DEPOOutStream << "; Golden = " << vecGear[GearGolden] << strUnit << "; High = " << vecGear[GearHigh] << strUnit << endl;
        if (GearLow >= 0)
        {
            DEPOOutStream << "ObjLow = " << vecMeasuredData[GearLow].Obj;
        } else
        {
            DEPOOutStream << "ObjLow = NaN";
        }
        DEPOOutStream << "; ObjGolden = " << vecMeasuredData[GearGolden].Obj << "; ObjHigh = " << vecMeasuredData[GearHigh].Obj << endl;

        if (vecMeasuredData[GearHigh].PerfLoss < DEPOCfg.PerfLossConstraint)
        { // 更新/保存一个用来保底的配置
            OptGear = GearHigh;
        } else
        {
            break; // 如果违反性能损失约束, 立即跳出
        }
        
        if (GearHigh - GearLow <= Error) // 搜索结束判定条件
        {
            // 找到 性能损失满足约束 且 目标函数值最小 的配置档位
            vector<int> vecCandidate;
            if (GearLow >=0 && vecMeasuredData[GearLow].PerfLoss < DEPOCfg.PerfLossConstraint) {
                vecCandidate.emplace_back(GearLow);
            }
            if (GearGolden >=0 && vecMeasuredData[GearGolden].PerfLoss < DEPOCfg.PerfLossConstraint) {
                vecCandidate.emplace_back(GearGolden);
            }
            if (GearHigh >=0 && vecMeasuredData[GearHigh].PerfLoss < DEPOCfg.PerfLossConstraint) {
                vecCandidate.emplace_back(GearHigh);
            }
            if (vecCandidate.size() == 0) break;

            float minObj = 1e6;
            for (auto Gear : vecCandidate)
            {
                if (vecMeasuredData[Gear].Obj < minObj)
                {
                    minObj = vecMeasuredData[Gear].Obj;
                    OptGear = Gear;
                }
            }
            break;
        }

        // 设置待测量的档位
        if (GearType == GEAR_TYPE::PWR_LMT)
        {
            NVMLConfig.SetPowerLimit(vecGear[currGear]);
        } else if (GearType == GEAR_TYPE::SM_CLK_LMT)
        {
            NVMLConfig.SetSMClkUpperLimit(vecGear[currGear]);
        } else
        {
            NVMLConfig.SetMemClkUpperLimit(vecGear[currGear]);
        }
        // 进行测量
        measureData(vecMeasuredData[currGear], MeasureWindowDuration, 0.3);
        CHECK_TERMINATION_BREAK;

        // 计算 优化目标函数值 和 性能损失
        vecMeasuredData[currGear].Gear = currGear;
        ObjFunc(vecMeasuredData[currGear], BaseData, DEPOCfg.OptObj, DEPOCfg.PerfLossConstraint);

        DEPOOutStream << std::dec << std::fixed << std::setprecision(2) << "DEPO: " 
        << strGearType << " = " << vecGear[vecMeasuredData[currGear].Gear] << strUnit << "; "
        << "PerfLoss = " << vecMeasuredData[currGear].PerfLoss * 100 << " %; "
        << "Obj = " << vecMeasuredData[currGear].Obj << "; "
        << "KnlPerSec = " << vecMeasuredData[currGear].KnlPerSec << " k/s; "
        << "NumKnls = " << vecMeasuredData[currGear].NumKnls << " k; "
        << "T = " << vecMeasuredData[currGear].T << " s; "
        << "avgPwr = " << vecMeasuredData[currGear].avgPwr << " W" << endl;
    }

    if (GearType == GEAR_TYPE::PWR_LMT)
    {
        NVMLConfig.SetPowerLimit(vecGear[OptGear]);
    } else if (GearType == GEAR_TYPE::SM_CLK_LMT)
    {
        NVMLConfig.SetSMClkUpperLimit(vecGear[OptGear]);
    } else
    {
        NVMLConfig.SetMemClkUpperLimit(vecGear[OptGear]);
    }

    DEPOOutStream << endl << "Optimal " << strGearType << " = " << vecGear[OptGear] << strUnit << endl << endl;
    return OptGear;
}

// 计算 目标函数 和 性能损失
void ObjFunc(MEASURED_DATA& Data, MEASURED_DATA& BaseData, OPTIMIZATION_OBJECTIVE OptObj, float PerfLossConstraint) {

    double tmpFactor = BaseData.T / Data.T; // 由于测量时长不同, 计算比例系数, 进行缩放修正

    Data.PerfLoss = BaseData.KnlPerSec / Data.KnlPerSec - 1;
    if (OptObj == OPTIMIZATION_OBJECTIVE::EDS)
    {
        double k = 1 + PerfLossConstraint;
        double EDS = 1.0/k * (BaseData.NumKnls/(Data.NumKnls*tmpFactor)) * ((k-1)*((Data.Eng*tmpFactor)/BaseData.Eng)+1);
        Data.Obj = EDS;
    } else if (OptObj == OPTIMIZATION_OBJECTIVE::ED2P)
    {
        double tmpEng = Data.Eng*tmpFactor / BaseData.Eng;
        double tmpT = Data.T*tmpFactor / BaseData.T;
        double tmpNumKnls = Data.NumKnls*tmpFactor / BaseData.NumKnls;
        double ED2P = (tmpEng * tmpT * tmpT) / (tmpNumKnls * tmpNumKnls * tmpNumKnls);
        Data.Obj = ED2P;
        if (PerfLossConstraint >= 0.0 && Data.PerfLoss > PerfLossConstraint)
        {
            Data.Obj += 1; // 性能损失超过阈值, 增大目标函数, 作为惩罚
        }
        
    }else if (OptObj == OPTIMIZATION_OBJECTIVE::EDP)
    {
        double tmpEng = Data.Eng*tmpFactor / BaseData.Eng;
        double tmpT = Data.T*tmpFactor / BaseData.T;
        double tmpNumKnls = Data.NumKnls*tmpFactor / BaseData.NumKnls;
        double EDP = (tmpEng * tmpT) / (tmpNumKnls * tmpNumKnls);
        Data.Obj = EDP;
        if (PerfLossConstraint >= 0.0 && Data.PerfLoss > PerfLossConstraint)
        {
            Data.Obj += 1; // 性能损失超过阈值, 增大目标函数, 作为惩罚
        }

    } else if (OptObj == OPTIMIZATION_OBJECTIVE::ENERGY)
    {
        double tmpEng = Data.Eng*tmpFactor / BaseData.Eng;
        double tmpNumKnls = Data.NumKnls*tmpFactor / BaseData.NumKnls;
        double Energy = tmpEng / tmpNumKnls;
        Data.Obj = Energy;
        if (PerfLossConstraint >= 0.0 && Data.PerfLoss > PerfLossConstraint)
        {
            Data.Obj += 1; // 性能损失超过阈值, 增大目标函数, 作为惩罚
        }

    } else
    {
        double tmpEng = Data.Eng*tmpFactor / BaseData.Eng;
        double tmpNumKnls = Data.NumKnls*tmpFactor / BaseData.NumKnls;
        double Energy = tmpEng / tmpNumKnls;
        Data.Obj = Energy;
        if (PerfLossConstraint >= 0.0 && Data.PerfLoss > PerfLossConstraint)
        {
            Data.Obj += 1; // 性能损失超过阈值, 增大目标函数, 作为惩罚
        }

    }
    Data.flagObj = true;
}

void measureData(MEASURED_DATA& Data,float MeasureDuration, float discount) {

    // 如果没测量到 kernel, 会 continue 再测一次
    while (!DEPOCfg.terminateThread)
    {
        // 清空测量数据缓冲区
        RawData.reset();

        // discount < 0.0 : RawData 累加到 Data
        // discount == 0.0 : RawData 覆盖 Data
        // discount > 0.0 : 对RawData进行衰减, RawData 和 Data 求加权平均

        struct timeval TimeStampBegin, TimeStampEnd;

        // 注册时钟信号
        // tv_usec 的取值范围 [0, 999999]
        long tmp_tv_sec = (unsigned int)DEPOCfg.SampleInterval;
        long tmp_tv_usec = ((unsigned int)(DEPOCfg.SampleInterval * 1e6) % (unsigned int)1e6);
        signal(SIGALRM, AlarmSampler);
        struct itimerval tick;
        tick.it_value.tv_sec = 0; // 0秒钟后将启动定时器
        tick.it_value.tv_usec = 1;
        tick.it_interval.tv_sec = tmp_tv_sec; // 定时器启动后，每隔 SampleInterval s 将执行相应的函数
        tick.it_interval.tv_usec = tmp_tv_usec;

        // std::cout << "measureData: tmp_tv_sec = " << std::dec << tmp_tv_sec << std::endl;
        // std::cout << "measureData: tmp_tv_usec = " << std::dec << tmp_tv_usec << std::endl;
        
        // 使能 kernel 计数
        DEPOCfg.KernelCountEnabled = 1;
        // 启动 NVML 测量
        std::cout << "measureData: 启动 NVML 测量" << std::endl;
        setitimer(ITIMER_REAL, &tick, NULL);
        gettimeofday(&TimeStampBegin, NULL);

        // 延时
        uint64_t TimeStep = 0.01 * 1e6;
        uint64_t RemainingTime = MeasureDuration * 1e6; // 剩余时间 us
        while (true) {
            // std::cout << "measureData: RemainingTime = " << std::dec << RemainingTime << std::endl;
            if (RemainingTime > TimeStep) {
                usleep(TimeStep);
                RemainingTime -= TimeStep;
            } else {
                usleep(RemainingTime);
                RemainingTime = 0;
                break;
            }

            // Check the condition again
            if (DEPOCfg.terminateThread) {
                tick.it_interval.tv_sec = 0; // 设置 interval 为 0, 完成本次定时之后, 停止定时
                tick.it_interval.tv_usec = 0;
                setitimer(ITIMER_REAL, &tick, NULL); // 完成本次定时之后，停止定时
                std::cout << "measureData: 停止 NVML 测量" << std::endl;
                std::cout << "measureData: 延时提前结束" << std::endl;
                return;
            }
        }
        if (DEPOCfg.terminateThread) {
            // 关闭 kernel 计数
            DEPOCfg.KernelCountEnabled = 0;
            return; // 如果停止标志为 true 直接返回
        } 

        // 停止 NVML 测量
        // 结束采样
        getitimer(ITIMER_REAL, &tick); // 获得本次定时的剩余时间
        tick.it_interval.tv_sec = 0; // 设置 interval 为 0, 完成本次定时之后, 停止定时
        tick.it_interval.tv_usec = 0;
        setitimer(ITIMER_REAL, &tick, NULL); // 完成本次定时之后，停止定时
        std::cout << "measureData: 停止 NVML 测量" << std::endl;
        // 关闭 kernel 计数
        DEPOCfg.KernelCountEnabled = 0;
        gettimeofday(&TimeStampEnd, NULL);

        double MeasureDurationActual = (TimeStampEnd.tv_sec - TimeStampBegin.tv_sec) + (TimeStampEnd.tv_usec - TimeStampBegin.tv_usec) * 1e-6;

        // 延时 保证 NVML 测量数据的完整性
        getitimer(ITIMER_REAL, &tick); // 获得本次定时的剩余时间
        tick.it_value.tv_usec += 5000; // 5ms
        usleep(tick.it_value.tv_usec);

        CHECK_TERMINATION_RETURN; // 如果停止标志为 true 直接跳出

        RawData.lock();

        for (auto& data : RawData.vecData)
        {
            RawData.NumKnls += data.NumKnls;
        }
        if (RawData.NumKnls == 0)
        {
            RawData.unlock();
            continue; // 如果没测量到 kernel, 就再测一次
        }

        RawData.T = MeasureDurationActual;
        RawData.avgPwr = RawData.sumPwr / RawData.nvml_measure_count;
        RawData.Eng = RawData.avgPwr * RawData.T;

        if (discount < 0.0) // discount < 0.0 : RawData 累加到 Data
        {
            Data.T += RawData.T;
            Data.Eng += RawData.Eng;
            Data.NumKnls += RawData.NumKnls;
            Data.KnlPerSec = Data.NumKnls / Data.T;
            Data.avgPwr = Data.Eng / Data.T;

        } else if (discount == 0.0 || Data.NumKnls == 0) // discount == 0.0 : RawData 覆盖 Data
        {
            Data.clear();
            Data.T = RawData.T;
            Data.Eng = RawData.Eng;
            Data.NumKnls += RawData.NumKnls;
            Data.KnlPerSec = Data.NumKnls / Data.T;
            Data.avgPwr = Data.Eng / Data.T;

        } else { // discount > 0.0 : 对RawData进行衰减, RawData 和 Data 求加权平均
            Data.T = (discount * Data.T + RawData.T) / (discount + 1);
            Data.Eng = (discount * Data.Eng + RawData.Eng) / (discount + 1);
            Data.NumKnls = (discount * Data.NumKnls + RawData.NumKnls) / (discount + 1);
            Data.KnlPerSec = Data.NumKnls / Data.T;
            Data.avgPwr = Data.Eng / Data.T;
        }
        Data.flagObj = false;
        RawData.unlock();
        break;

    }

    return;
}

void measureBaseData(MEASURED_DATA& Data, float MeasureDuration, float discount) {
    // discount < 0.0 : RawData 累加到 Data
    // discount == 0.0 : RawData 覆盖 Data
    // discount > 0.0 : 对RawData进行衰减, RawData 和 Data 求加权平均

    std::cout << "measureBaseData: 进入" << std::endl;
    NVMLConfig.ResetPowerLimit();
    NVMLConfig.ResetSMClkLimit();
    NVMLConfig.ResetMemClkLimit();
    sleep(1);
    CHECK_TERMINATION_RETURN;

    measureData(Data, MeasureDuration, discount);
}

void callbackHandler(void * userdata, CUpti_CallbackDomain domain, CUpti_CallbackId cbid, void const * cbdata)
{
    if (DEPOCfg.KernelCountEnabled == 0)
    {
        return;
    }

    const CUpti_CallbackData *cbInfo = (CUpti_CallbackData *)cbdata;

    // Check last error
    CUPTI_API_CALL(cuptiGetLastError());

    if (domain == CUPTI_CB_DOMAIN_RESOURCE && cbid == CUPTI_CBID_RESOURCE_CONTEXT_CREATED) 
    {
        CUpti_ResourceData const * res_data = static_cast<CUpti_ResourceData const *>(cbdata);
        CUcontext ctx = res_data->context;

        // MEASURED_DATA tmpData;
        // tmpData.ctx = ctx;

        // If valid for profiling, set up profiler and save to shared structure
        // Update shared structures
        // currCtx = ctx;
        // mapMeasuredData[ctx].ctx = ctx;

        cout << "callbackHandler: CUPTI_CBID_RESOURCE_CONTEXT_CREATED ctx = " << std::hex << ctx << endl;

    } else if (domain == CUPTI_CB_DOMAIN_RESOURCE && cbid == CUPTI_CBID_RESOURCE_CONTEXT_DESTROY_STARTING)
    {
        CUpti_ResourceData const * res_data = static_cast<CUpti_ResourceData const *>(cbdata);
        CUcontext ctx = res_data->context;

        // Update shared structures
        // mapMeasuredData.erase(ctx);

        cout << "callbackHandler: CUPTI_CBID_RESOURCE_CONTEXT_DESTROY_STARTING ctx = " << std::hex << ctx << endl;

    } else if (domain == CUPTI_CB_DOMAIN_DRIVER_API && cbid == CUPTI_DRIVER_TRACE_CBID_cuLaunchKernel)
    {   // For a driver call to launch a kernel:

        CUpti_CallbackData const * data = static_cast<CUpti_CallbackData const *>(cbdata);
        CUcontext ctx = data->context;
        const cuLaunchKernel_params_st* pParams = (const cuLaunchKernel_params_st*)data->functionParams;
        CUstream tmpStream = pParams->hStream;

        // On entry, enable / update profiling as needed
        if (data->callbackSite == CUPTI_API_ENTER)
        {
            // Check for this context in the configured contexts
            // If not configured, it isn't compatible with profiling

            int tmpIndex = RawData.indexIdle;
            RawData.indexIdle = (tmpIndex+1)%RawData.BufLen;
            int lockedIndex = -1;

            // 在多个位置尝试锁定
            int i = 0;
            for (i = 0; i < RawData.BufLen; i++)
            {
                if (RawData.queLock[(tmpIndex+i)%RawData.BufLen].try_lock()) {
                    RawData.indexIdle = (tmpIndex+i+1)%RawData.BufLen;
                    lockedIndex = (tmpIndex+i)%RawData.BufLen;
                    break;
                } else
                {
                    RawData.indexIdle = (tmpIndex+i+2)%RawData.BufLen;
                }
            }

            // 在一个位置等待锁定
            if (lockedIndex < 0) {
                RawData.queLock[tmpIndex%RawData.BufLen].lock();
                lockedIndex = tmpIndex%RawData.BufLen;
            }

            RawData.vecData[lockedIndex].NumKnls++; // kernel 计数 +1

            RawData.queLock[lockedIndex].unlock(); // 解锁
            
        }
    }

    return;
}

static void GlobalInit(void) {
    // 从环境变量读取 输出文件路径 和 配置文件路径
    char *pOutPath;
    char *pConfigPath;
    // CUDA_INJECTION64_PATH
    // DEPO_OUTPUT_PATH
    // DEPO_CONFIG_PATH
    pOutPath = getenv("DEPO_OUTPUT_PATH");
    if (pOutPath != NULL) {
        DEPOCfg.OutPath = pOutPath;
    } else {
        DEPOCfg.OutPath = "./DEPO_out.txt";
    }
    std::cout << "GlobalInit: DEPO_OUTPUT_PATH = " << DEPOCfg.OutPath << std::endl;
    
    pConfigPath = getenv("DEPO_CONFIG_PATH");
    if (pConfigPath != NULL) {
        DEPOCfg.ConfigPath = pConfigPath;
    } else {
        DEPOCfg.ConfigPath = "";
    }
    std::cout << "GlobalInit: DEPO_CONFIG_PATH = " << DEPOCfg.ConfigPath << std::endl;

    //从配置文件中初始化参数
    vecMemClk.clear();
    vecSMClk.clear();
    vecPwrLmt.clear();
    std::ifstream fin(DEPOCfg.ConfigPath);
    std::string line;
    while (std::getline(fin, line))
    {
        // cout << "line = " << line << endl;
        line=erase_note(line);
        line=trim(line);
        if(line=="")
        {
            continue;
        }
        int pos = line.find_first_of(':');
        std::string key=line.substr(0, pos);
        std::string value=line.substr(pos+1,line.length()-pos-1);
        key=trim(key);
        value=trim(value);
        // cout << "key = " << key << endl;
        // cout << "value = " << value << endl;
        if(key=="GPU Name")
        {
            DEPOCfg.GPUName=value.c_str();
        }
        else if(key=="GPU ID")
        {
            DEPOCfg.GPUID=atoi(value.c_str());
        }
        else if(key=="Default Power Limit")
        {
            DEPOCfg.DefaultPowerLimit=atoi(value.c_str());
        }
        else if(key=="Max SM Clock")
        {
            DEPOCfg.MaxSMClk=atoi(value.c_str());
        }
        else if(key=="Max Memory Clock")
        {
            DEPOCfg.MaxMemClk=atoi(value.c_str());
        }
        else if(key=="SM Clocks")
        {
            int pos=value.find_first_of(' ');
            if(pos==std::string::npos)
            {
                vecSMClk.push_back(atoi(value.c_str()));
            }
            else
            {
                std::istringstream iss(value);
                std::string token="";
                while (getline(iss, token,' '))
                {
                    vecSMClk.push_back(atoi(token.c_str()));
                }
                sort(vecSMClk.begin(),vecSMClk.end());
            }
        }
        else if(key=="Memory Clocks")
        {
            int pos=value.find_first_of(' ');
            if(pos==std::string::npos)
            {   
                vecMemClk.push_back(atoi(value.c_str()));
            }
            else
            {
                std::istringstream iss(value);
                std::string token="";
                while (getline(iss, token,' '))
                {
                    vecMemClk.push_back(atoi(token.c_str()));
                }
                sort(vecMemClk.begin(),vecMemClk.end());
            }
        }
        else if(key=="Power Limits")
        {
            int pos=value.find_first_of(' ');
            if(pos==std::string::npos)
            {   
                vecPwrLmt.push_back(atoi(value.c_str()));
            }
            else
            {
                std::istringstream iss(value);
                std::string token="";
                while (getline(iss, token,' '))
                {
                    vecPwrLmt.push_back(atoi(token.c_str()));
                }
                sort(vecPwrLmt.begin(),vecPwrLmt.end());
            }
        }
        else if(key=="Number of SMs"){
            DEPOCfg.NumSMs=atoi(value.c_str());
        }
        else if(key=="Number of CUDA Cores per SM"){
            DEPOCfg.CoresPerSM=atoi(value.c_str());
        }
        else if(key=="Total Number of CUDA Cores"){
            DEPOCfg.NumCores=atoi(value.c_str());
        }
        else if(key=="Performance Loss Constraint"){
            DEPOCfg.PerfLossConstraint=atof(value.c_str())/100.0;
        }
        else if(key=="Optimization Objective"){
            if(value=="Energy")
            {
                DEPOCfg.OptObj=OPTIMIZATION_OBJECTIVE::ENERGY;
            }
            else if(value=="EDP")
            {
                DEPOCfg.OptObj=OPTIMIZATION_OBJECTIVE::EDP;
            }
            else if(value=="ED2P")
            {
                DEPOCfg.OptObj=OPTIMIZATION_OBJECTIVE::ED2P;
            }
            else if(value=="EDS")
            {
                DEPOCfg.OptObj=OPTIMIZATION_OBJECTIVE::EDS;
            }
            else 
            {
                DEPOCfg.OptObj=OPTIMIZATION_OBJECTIVE::ENERGY;
            }
        }
        else if(key=="Initial Measurement Duration"){
                DEPOCfg.MeasureWindowDuration=atof(value.c_str());
        }
        else if(key=="Sample Interval"){
                DEPOCfg.SampleInterval=atof(value.c_str());
        }

    }
    fin.close();
    show();

    // 从环境变量读取工作模式 DEPO_WORK_MODE
    char *pWorkMode;
    std::string tmpStr = "DEPO";
    pWorkMode = getenv("DEPO_WORK_MODE");
    if (pWorkMode != NULL) {
        tmpStr = pWorkMode;
        if (tmpStr == "DEPO")
        {
            DEPOCfg.WorkMode=WORK_MODE::DEPO;
        } else if (tmpStr == "STATIC")
        {
            DEPOCfg.WorkMode=WORK_MODE::STATIC;
        } else if (tmpStr == "MEASURE")
        {
            DEPOCfg.WorkMode=WORK_MODE::MEASURE;
        } else if (tmpStr == "EVALUATE")
        {
            DEPOCfg.WorkMode=WORK_MODE::EVALUATE;
        } else
        {
            DEPOCfg.WorkMode=WORK_MODE::DEPO;
        }
    } else {
        DEPOCfg.WorkMode = WORK_MODE::DEPO;
    }
    std::cout << "GlobalInit: DEPO_WORK_MODE = " << tmpStr << std::endl;

    // 从环境变量读取 TestedSMClk 和 TestedMemClk
    char *pTestedSMClk;
    pTestedSMClk = getenv("TESTED_SM_CLK");
    if (pTestedSMClk != NULL) {
        TestedSMClk = atoi(pTestedSMClk);
    } else {
        TestedSMClk = DEPOCfg.MaxSMClk;
    }
    std::cout << "GlobalInit: TESTED_SM_CLK = " << TestedSMClk << std::endl;

    char *pTestedMemClk;
    pTestedMemClk = getenv("TESTED_MEM_CLK");
    if (pTestedMemClk != NULL) {
        TestedMemClk = atoi(pTestedMemClk);
    } else {
        TestedMemClk = DEPOCfg.MaxMemClk;
    }
    std::cout << "GlobalInit: TESTED_MEM_CLK = " << TestedMemClk << std::endl;

    char *pTestedPwrLmt;
    pTestedPwrLmt = getenv("TESTED_PWR_LMT");
    if (pTestedPwrLmt != NULL) {
        TestedPwrLmt = atoi(pTestedPwrLmt);
    } else {
        TestedPwrLmt = DEPOCfg.DefaultPowerLimit;
    }
    std::cout << "GlobalInit: TESTED_PWR_LMT = " << TestedPwrLmt << std::endl;


    DEPOCfg.StartTimeStamp = 0.0; // 开始的绝对时间戳
    // 初始化测量启停控制相关全局变量
    DEPOCfg.initialized = 0;
    DEPOCfg.subscriber = 0;
    DEPOCfg.detachCupti = 0;
    DEPOCfg.tracingEnabled = 0;
    DEPOCfg.isMeasuring = 0;
    DEPOCfg.istimeout = 0;
    DEPOCfg.mutexFinalize = PTHREAD_MUTEX_INITIALIZER;
    DEPOCfg.mutexCondition = PTHREAD_COND_INITIALIZER;
    std::cout << "\nDEPOCfg.GPUID " << DEPOCfg.GPUID << std::endl;
    std::cout << "DEPOCfg.NumCores" << DEPOCfg.NumCores << std::endl;

    NVMLConfig.init(DEPOCfg.GPUID); // 初始化 NVML


    if (vecSMClk.size() == 0 || vecMemClk.size() == 0 || vecPwrLmt.size() == 0) {
        std::cout << "GlobalInit: Initialization Error" << std::endl;
        DEPOOutStream << "GlobalInit: Initialization Error" << std::endl;
        DEPOCfg.terminateThread = 1; // 设置应用结束标志为 true
    } else {
        DEPOCfg.terminateThread = 0; // 设置应用结束标志为 false
    }
    
}

static void atExitHandler(void) {
    std::cout << "atExitHandler: 进入" << std::endl;
    DEPOCfg.terminateThread = 1;

    CUPTI_API_CALL(cuptiGetLastError());

    // 关闭定时器
    struct itimerval tick;
    tick.it_interval.tv_sec = 0; // 设置 interval 为 0, 完成本次定时之后, 停止定时
    tick.it_interval.tv_usec = 0;
    setitimer(ITIMER_REAL, &tick, NULL); // 完成本次定时之后，停止定时
    // 延时 保证 NVML 测量数据的完整性
    // getitimer(ITIMER_REAL, &tick); // 获得本次定时的剩余时间
    // tick.it_value.tv_usec += 10000; // +10ms 再进行延时
    // usleep(tick.it_value.tv_usec);
    usleep(0.1 * 1e6);
    std::cout << "atExitHandler: 关闭定时器1" << std::endl;

    std::cout << "cuptiFinalize()" << std::endl;
    CUPTI_API_CALL(cuptiFinalize());
    
    // 等待工作线程结束
    if (DEPOCfg.dynamicThread != 0) {
        PTHREAD_CALL(pthread_join(DEPOCfg.dynamicThread, NULL));
        std::cout << "atExitHandler: pthread_join" << std::endl;
    }

    // 重置能耗配置
    NVMLConfig.ResetPowerLimit();
    NVMLConfig.ResetSMClkLimit();
    NVMLConfig.ResetMemClkLimit();
    // 结束 NVML
    NVMLConfig.uninit();
    // 关闭输出文件
    DEPOOutStream.close();

    std::cout << "atExitHandler: 结束" << std::endl;
}

void registerAtExitHandler(void) {
    // Register atExitHandler
    atexit(&atExitHandler);
}

int STDCALL
InitializeInjection(void) {

    printf("\nstart injection\n\n");

    if (DEPOCfg.initialized) {
        return 1;
    }
    DEPOCfg.initialized = 1;
    GlobalInit();
    registerAtExitHandler(); // 注册 注入结束时 要执行的函数

    // Initialize Mutex
    PTHREAD_CALL(pthread_mutex_init(&DEPOCfg.mutexFinalize, 0));

    // 创建并打开输出文件
    DEPOOutStream.open(DEPOCfg.OutPath, std::ifstream::out);
    DEPOOutStream.close();
    // 再以 读写 方式 打开已经存在的文件
    DEPOOutStream.open(DEPOCfg.OutPath, std::ifstream::out | std::ifstream::in | std::ifstream::binary);
    if(!DEPOOutStream.is_open()){
        DEPOOutStream.close();
        std::cout << "ERROR: failed to open output file" << std::endl;
        std::cout << "DEPOCfg.OutPath: " << DEPOCfg.OutPath << std::endl;
        return 1;
    }

    if (DEPOCfg.WorkMode == WORK_MODE::DEPO) {

        DEPOOutStream << "-------- Dynamic Energy-Performance Optimiser Output --------" << std::endl;
        DEPOOutStream << std::endl;

        // 输出绝对起始时间戳
        struct timeval tmpTimeStamp;
        gettimeofday(&tmpTimeStamp, NULL);
        DEPOCfg.StartTimeStamp = (double)tmpTimeStamp.tv_sec + (double)tmpTimeStamp.tv_usec * 1e-6;
        DEPOOutStream << "Start Time Stamp: " << std::fixed << std::setprecision(2) << DEPOCfg.StartTimeStamp << " s" << std::endl;
        DEPOOutStream << std::endl;

        DEPOOutStream << std::endl;
        DEPOOutStream << "Relative Time Stamp: " << std::fixed << std::setprecision(2) << (double)0.0 << std::endl;
        DEPOOutStream << std::endl;

        DEPOOutStream << "WORK_MODE::DEPO" << std::endl;
        PTHREAD_CALL(pthread_create(&DEPOCfg.dynamicThread, NULL, DEPOMain, NULL));

    } else if (DEPOCfg.WorkMode == WORK_MODE::EVALUATE)
    {
        DEPOOutStream << "-------- Evaluate Performance Loss --------" << std::endl;
        DEPOOutStream << std::endl;

        // 输出绝对起始时间戳
        struct timeval tmpTimeStamp;
        gettimeofday(&tmpTimeStamp, NULL);
        DEPOCfg.StartTimeStamp = (double)tmpTimeStamp.tv_sec + (double)tmpTimeStamp.tv_usec * 1e-6;
        DEPOOutStream << "Start Time Stamp: " << std::fixed << std::setprecision(2) << DEPOCfg.StartTimeStamp << " s" << std::endl;
        DEPOOutStream << std::endl;

        DEPOOutStream << std::endl;
        DEPOOutStream << "Relative Time Stamp: " << std::fixed << std::setprecision(2) << (double)0.0 << std::endl;
        DEPOOutStream << std::endl;

        DEPOOutStream << "WORK_MODE::EVALUATE" << std::endl;
        PTHREAD_CALL(pthread_create(&DEPOCfg.dynamicThread, NULL, EvaluatePerformance, NULL));
    }
    

    return 1;
}

//删除空格
std::string trim(std::string s)
{
    if(s.empty())return s;
    s.erase(0,s.find_first_not_of(" \n\r"));
    s.erase(s.find_last_not_of(" \n\r")+1);
    return s;
}
//去除#注释
std::string erase_note(std::string s)
{
    if(s.empty())return s;
    int pos = s.find_first_of('#');
    s=s.substr(0, pos);
    return s;
}

void show(){
    std::cout << "show: DEPO_CONFIG_PATH = " << DEPOCfg.ConfigPath << std::endl;

    std::cout << "vecPwrLmt: " << std::dec;
    for (unsigned int i = 0; i < vecPwrLmt.size(); i++) { // 100 350
       std::cout<<vecPwrLmt[i]<<" ";
    }
    std::cout<<std::endl;
    std::cout << "vecSMClk: " << std::dec;
    for (unsigned int i = 0; i < vecSMClk.size(); i++) { // 210 2160
       std::cout<<vecSMClk[i]<<" ";
    }
    std::cout<<std::endl;
    std::cout << "vecMemClk: " << std::dec;
    for (unsigned int i = 0; i < vecMemClk.size(); i++) { // 210 2160
       std::cout<<vecMemClk[i]<<" ";
    }
    std::cout<<std::endl;

    std::cout << "DEPOCfg.WorkMode = ";
    switch (DEPOCfg.WorkMode)
    {
    case WORK_MODE::DEPO:
        cout << "DEPO" << endl;
        break;
    case WORK_MODE::MEASURE:
        cout << "MEASURE" << endl;
        break;
    case WORK_MODE::STATIC:
        cout << "STATIC" << endl;
        break;
    case WORK_MODE::EVALUATE:
        cout << "EVALUATE" << endl;
        break;
    default:
        cout << endl;
        break;
    }
    std::cout << "DEPOCfg.OptObj = ";
    switch (DEPOCfg.OptObj)
    {
    case OPTIMIZATION_OBJECTIVE::ENERGY:
        cout << "ENERGY" << endl;
        break;
    case OPTIMIZATION_OBJECTIVE::EDP:
        cout << "EDP" << endl;
        break;
    case OPTIMIZATION_OBJECTIVE::ED2P:
        cout << "ED2P" << endl;
        break;
    case OPTIMIZATION_OBJECTIVE::EDS:
        cout << "EDS" << endl;
        break;
    default:
        cout << endl;
        break;
    }
    std::cout << "DEPOCfg.PerfLossConstraint = " << DEPOCfg.PerfLossConstraint << std::endl;
    std::cout << "DEPOCfg.GPUID = " << DEPOCfg.GPUID << std::endl;
    std::cout << "DEPOCfg.NumCores = " << DEPOCfg.NumCores << std::endl;
    std::cout << "DEPOCfg.DefaultPowerLimit = " << DEPOCfg.DefaultPowerLimit << std::endl;
    std::cout << "DEPOCfg.NumSMs = " << DEPOCfg.NumSMs << std::endl;
    std::cout << "DEPOCfg.MaxSMClk = " << DEPOCfg.MaxSMClk << std::endl;
    std::cout << "DEPOCfg.MaxMemClk = " << DEPOCfg.MaxMemClk << std::endl;
    std::cout << "DEPOCfg.SampleInterval = " << DEPOCfg.SampleInterval << std::endl;
    std::cout << "DEPOCfg.MeasureWindowDuration = " << DEPOCfg.MeasureWindowDuration << std::endl;
    std::cout << std::endl;
}

