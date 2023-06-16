/*
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 *
 * Sample CUPTI based injection to attach and detach CUPTI
 * For detaching, it uses CUPTI API cuptiFinalize().
 *
 * It is recommended to invoke API cuptiFinalize() in the 
 * exit callsite of any CUDA Driver/Runtime API.
 * 
 * API cuptiFinalize() destroys and cleans up all the
 * resources associated with CUPTI in the current process.
 * After CUPTI detaches from the process, the process will 
 * keep on running with no CUPTI attached to it.
 * 
 * CUPTI can be attached by calling any CUPTI API as CUPTI 
 * supports lazy initialization. Any subsequent CUPTI API 
 * call will reinitialize the CUPTI. 
 * 
 * You can attach and detach CUPTI any number of times.
 * 
 * After building the sample, set the following environment variable
 * export CUDA_INJECTION64_PATH=<full_path>/libCuptiFinalize.so
 * Add CUPTI library in LD_LIBRARY_PATH and run any CUDA sample
 * with runtime more than 10 sec for demonstration of the
 * CUPTI sample
 */

/*******************************************************************************
Copyright(C), 2022-2022, 瑞雪轻飏
     FileName: libGPOEO.cpp
       Author: 瑞雪轻飏
      Version: 0.01
Creation Date: 20220613
  Description: MFGPOEO 主文件 实现注入 和 反馈控制算法
       Others: 
*******************************************************************************/

#include "libGPOEO.h"

// Function Definitions
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
    std::cout << "show: MFGPOEO_CONFIG_PATH = " << GlobalConfig.ConfigPath << std::endl;

    std::cout << "vecSMClk: ";
    for (unsigned int i = 0; i < vecSMClk.size(); i++) { // 210 2160
       std::cout<<vecSMClk[i]<<" ";
    }
    std::cout<<std::endl;
    std::cout << "vecMemClk: ";
    for (unsigned int i = 0; i < vecMemClk.size(); i++) { // 210 2160
       std::cout<<vecMemClk[i]<<" ";
    }
    std::cout<<std::endl;

    std::cout << "GlobalConfig.gpu_id = " << GlobalConfig.gpu_id << std::endl;
    std::cout << "GlobalConfig.NumCores = " << GlobalConfig.NumCores << std::endl;
    std::cout << "GlobalConfig.DefaultPowerLimit = " << GlobalConfig.DefaultPowerLimit << std::endl;
    std::cout << "GlobalConfig.NumSMs = " << GlobalConfig.NumSMs << std::endl;
    std::cout << "GlobalConfig.MaxSMClk = " << GlobalConfig.MaxSMClk << std::endl;
    std::cout << "GlobalConfig.MaxMemClk = " << GlobalConfig.MaxMemClk << std::endl;
    std::cout << "GlobalConfig.kp = " << GlobalConfig.kp << std::endl;
    std::cout << "GlobalConfig.ki = " << GlobalConfig.ki << std::endl;
    std::cout << "GlobalConfig.kd = " << GlobalConfig.kd << std::endl;
    std::cout << "GlobalConfig.ExeTimeRadius = " << GlobalConfig.ExeTimeRadius << std::endl;
    std::cout << "GlobalConfig.GapTimeRadius = " << GlobalConfig.GapTimeRadius << std::endl;
    std::cout << "GlobalConfig.MeasureWindowDuration = " << GlobalConfig.MeasureWindowDuration << std::endl;
    std::cout << "GlobalConfig.WindowDurationMin = " << GlobalConfig.WindowDurationMin << std::endl;
    std::cout << "GlobalConfig.WindowDurationMax = " << GlobalConfig.WindowDurationMax << std::endl;
    std::cout << "GlobalConfig.Pct_Inc = " << GlobalConfig.Pct_Inc << std::endl;
    std::cout << "GlobalConfig.Pct_Ctrl = " << GlobalConfig.Pct_Ctrl << std::endl;
    std::cout << "GlobalConfig.BaseUpdateCycle = " << GlobalConfig.BaseUpdateCycle << std::endl;
    std::cout << "GlobalConfig.NumOptConfig = " << GlobalConfig.NumOptConfig << std::endl;
    std::cout << "GlobalConfig.StableSMClkStepRange = " << GlobalConfig.StableSMClkStepRange << std::endl;
    std::cout << "GlobalConfig.PhaseWindow = " << GlobalConfig.PhaseWindow << std::endl;
    std::cout << "GlobalConfig.PhaseChangeThreshold = " << GlobalConfig.PhaseChangeThreshold << std::endl;
}
static
void globalInit(void) {
    // 从环境变量读取 输出文件路径 和 配置文件路径
    char *pOutPath;
    char *pConfigPath;
    // CUDA_INJECTION64_PATH
    // MFGPOEO_OUTPUT_PATH
    // MFGPOEO_CONFIG_PATH
    pOutPath = getenv("MFGPOEO_OUTPUT_PATH");
    if (pOutPath != NULL) {
        GlobalConfig.OutPath = pOutPath;
    } else {
        GlobalConfig.OutPath = "./MFGPOEO_out.txt";
    }
    std::cout << "globalInit: MFGPOEO_OUTPUT_PATH = " << GlobalConfig.OutPath << std::endl;
    
    pConfigPath = getenv("MFGPOEO_CONFIG_PATH");
    if (pConfigPath != NULL) {
        GlobalConfig.ConfigPath = pConfigPath;
    } else {
        GlobalConfig.ConfigPath = "";
    }

    //从配置文件中初始化参数
    vecMemClk.clear();
    vecSMClk.clear();
    std::ifstream fin(GlobalConfig.ConfigPath);
    std::string line;
    while (std::getline(fin, line))
    {   
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
        if(key=="GPU Name")
        {
            GlobalConfig.GPUName=value.c_str();
        }
        else if(key=="GPU ID")
        {
            GlobalConfig.gpu_id=atoi(value.c_str());
        }
        else if(key=="Default Power Limit")
        {
            GlobalConfig.DefaultPowerLimit=atoi(value.c_str());
        }
        else if(key=="Max SM Clock")
        {
            GlobalConfig.MaxSMClk=atoi(value.c_str());
        }
        else if(key=="Max Memory Clock")
        {
            GlobalConfig.MaxMemClk=atoi(value.c_str());
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
        else if(key=="Number of SMs"){
            GlobalConfig.NumSMs=atoi(value.c_str());
        }
        else if(key=="Number of CUDA Cores per SM"){
            GlobalConfig.CoresPerSM=atoi(value.c_str());
        }
        else if(key=="Total Number of CUDA Cores"){
            GlobalConfig.NumCores=atoi(value.c_str());
        }
        else if(key=="Performance Loss Constraint"){
            GlobalConfig.objective=atof(value.c_str())/100.0;
        }
        else if(key=="Optimization Objective"){
            if(value=="Energy")
            {
                GlobalConfig.OptObj=OPTIMIZATION_OBJECTIVE::ENERGY;
            }
            else if(value=="EDP")
            {
                GlobalConfig.OptObj=OPTIMIZATION_OBJECTIVE::EDP;
            }
            else if(value=="ED2P")
            {
                GlobalConfig.OptObj=OPTIMIZATION_OBJECTIVE::ED2P;
            } else 
            {
                GlobalConfig.OptObj=OPTIMIZATION_OBJECTIVE::ENERGY;
            }
        }
        else if(key=="Kp"){
                GlobalConfig.kp=atof(value.c_str());
        }
        else if(key=="Ki"){
                GlobalConfig.ki=atof(value.c_str());
        }
        else if(key=="Kd"){
                GlobalConfig.kd=atof(value.c_str());
        }
        else if(key=="Execution Time Judgment Radius"){
                GlobalConfig.ExeTimeRadius=atof(value.c_str());
        }
        else if(key=="Gap Time Judgment Radius"){
                GlobalConfig.GapTimeRadius=atof(value.c_str());
        }
        else if(key=="Initial Measurement Duration"){
                GlobalConfig.MeasureWindowDuration=atof(value.c_str());
        }
        else if(key=="Minimum Measurement Duration"){
                GlobalConfig.WindowDurationMin=atof(value.c_str());
        }
        else if(key=="Maximum Measurement Duration"){
                GlobalConfig.WindowDurationMax=atof(value.c_str());
        }
        else if(key=="Duration Increase Threshold"){
                GlobalConfig.Pct_Inc=atof(value.c_str());
        }
        else if(key=="Optimization Execution Threshold"){
                GlobalConfig.Pct_Ctrl=atof(value.c_str());
        }
        else if(key=="Baseline Update Frequency")
        {
            GlobalConfig.BaseUpdateCycle=atoi(value.c_str());
        }
        else if(key=="Stability Judgment Window")
        {
            GlobalConfig.NumOptConfig=atoi(value.c_str());
        }
        else if(key=="SM Clock Gear Range")
        {
            GlobalConfig.StableSMClkStepRange=atoi(value.c_str());
        }
        else if(key=="Phase Detection Window")
        {
            GlobalConfig.PhaseWindow=atoi(value.c_str());
        }
        else if(key=="Phase Change Threshold")
        {
            GlobalConfig.PhaseChangeThreshold=atof(value.c_str());
        }
    }
    fin.close();
    show();

    // 从环境变量读取工作模式 MFGPOEO_WORK_MODE
    char *pWorkMode;
    std::string tmpStr = "PID";
    pWorkMode = getenv("MFGPOEO_WORK_MODE");
    if (pWorkMode != NULL) {
        tmpStr = pWorkMode;
        if (tmpStr == "PID")
        {
            GlobalConfig.WorkMode=WORK_MODE::PID;
        } else if (tmpStr == "STATIC")
        {
            GlobalConfig.WorkMode=WORK_MODE::STATIC;
        } else if (tmpStr == "KERNEL")
        {
            GlobalConfig.WorkMode=WORK_MODE::KERNEL;
        } else if (tmpStr == "PREDICT")
        {
            GlobalConfig.WorkMode=WORK_MODE::PREDICT;
        } else
        {
            GlobalConfig.WorkMode=WORK_MODE::PID;
        }
    } else {
        GlobalConfig.WorkMode = WORK_MODE::PID;
    }
    std::cout << "globalInit: MFGPOEO_WORK_MODE = " << tmpStr << std::endl;
    
    // 从环境变量读取 TestedSMClk 和 TestedMemClk
    char *pTestedSMClk;
    pTestedSMClk = getenv("TESTED_SM_CLK");
    if (pTestedSMClk != NULL) {
        TestedSMClk = atoi(pTestedSMClk);
    } else {
        TestedSMClk = GlobalConfig.MaxSMClk;
    }
    std::cout << "globalInit: TESTED_SM_CLK = " << TestedSMClk << std::endl;

    char *pTestedMemClk;
    pTestedMemClk = getenv("TESTED_MEM_CLK");
    if (pTestedMemClk != NULL) {
        TestedMemClk = atoi(pTestedMemClk);
    } else {
        TestedMemClk = GlobalConfig.MaxMemClk;
    }
    std::cout << "globalInit: TESTED_MEM_CLK = " << TestedMemClk << std::endl;

    // GlobalConfig.objective -= GlobalConfig.ObjRelax; // 适当减小性能损失约束, 即留一些余量, 减小违反原始约束的可能

    GlobalConfig.StartTimeStamp = 0.0; // 开始的绝对时间戳
    // 初始化测量启停控制相关全局变量
    GlobalConfig.initialized = 0;
    GlobalConfig.subscriber = 0;
    GlobalConfig.detachCupti = 0;
    GlobalConfig.tracingEnabled = 0;
    GlobalConfig.isMeasuring = 0;
    GlobalConfig.istimeout = 0;
    GlobalConfig.RefCountAnalysis = 0;
    GlobalConfig.mutexFinalize = PTHREAD_MUTEX_INITIALIZER;
    GlobalConfig.mutexCondition = PTHREAD_COND_INITIALIZER;
    std::cout << "\nGlobalConfig.gpu_id " << GlobalConfig.gpu_id << std::endl;
    std::cout << "GlobalConfig.NumCores" << GlobalConfig.NumCores << std::endl;
    // 为测量数据预留空间
    vecMeasureData.resize(VECTOR_MEASURE_DATA_SIZE);
    for (int i = 0; i < VECTOR_MEASURE_DATA_SIZE; ++i) {
        vecMeasureData[i].clear();
    }

    // 初始化测量模式所需缓冲
    vecAnalysisArgs.resize(VECTOR_MEASURE_DATA_SIZE);

    // 初始化 CUPTI 测量相关数据缓冲
    CUPTIBufferManager.init(VECTOR_MEASURE_DATA_SIZE, CUPTI_BUF_SIZE);

    NVMLConfig.init(GlobalConfig.gpu_id); // 初始化 NVML
    NVMLConfig.ResetSMClkLimit();
    NVMLConfig.ResetMemClkLimit();
    NVMLConfig.ResetPowerLimit();

    if (vecSMClk.size() == 0 || vecMemClk.size() == 0) {
        std::cout << "globalInit: Initialization Error" << std::endl;
        MFGPOEOOutStream << "globalInit: Initialization Error" << std::endl;
        GlobalConfig.terminateThread = 1; // 设置应用结束标志为 true
    } else {
        GlobalConfig.terminateThread = 0; // 设置应用结束标志为 false
    }

}

void registerAtExitHandler(void) {
    // Register atExitHandler
    atexit(&atExitHandler);
}

static void 
atExitHandler(void) {
    std::cout << "atExitHandler: 进入" << std::endl;
    GlobalConfig.terminateThread = 1;
    
    // 如果还在进行最后一次测量结束 或者 最后一次测量没有正常结束
    if (GlobalConfig.isMeasuring == 1) {
        std::cout << "atExitHandler: 还在进行最后一次测量..." << std::endl;

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

        if (GlobalConfig.tracingEnabled == 1) {
            GlobalConfig.tracingEnabled = 0;
            GlobalConfig.detachCupti = 1;
            GlobalConfig.istimeout = 1;
            CUPTI_CALL(cuptiActivityFlushAll(1));
            std::cout << "callbackHandler: cuptiFinalize()" << std::endl;
            CUPTI_CALL(cuptiFinalize());
            PTHREAD_CALL(pthread_cond_broadcast(&GlobalConfig.mutexCondition));
        }
        
        // // 如果 CUPTI 还在运行
        // if (GlobalConfig.tracingEnabled == 1) {
        //     std::cout << "atExitHandler: 杀死工作线程" << std::endl;
        //     int res_kill = pthread_kill(GlobalConfig.dynamicThread, 0);//0 signal is retain sign,then no signal is send 
        //     if(res_kill == ESRCH)
        //     {
        //         std::cout << "atExitHandler: pthread_kill: tid 无效" << std::endl;
        //     } else if (res_kill == 0) {
        //         GlobalConfig.dynamicThread = 0;
        //         std::cout << "atExitHandler: 杀死工作线程 完成" << std::endl;
        //     }
        //     PTHREAD_CALL(pthread_cond_broadcast(&GlobalConfig.mutexCondition));
        //     CUPTI_CALL(cuptiFinalize());
        //     std::cout << "atExitHandler: cuptiFinalize" << std::endl;
        // }
        GlobalConfig.tracingEnabled = 0;
        GlobalConfig.istimeout = 0;
        GlobalConfig.detachCupti = 0;
        GlobalConfig.subscriber = 0;

        GlobalConfig.isMeasuring = 0;
    }
    
    // 等待工作线程结束
    if (GlobalConfig.dynamicThread != 0) {
        PTHREAD_CALL(pthread_join(GlobalConfig.dynamicThread, NULL));
        std::cout << "atExitHandler: pthread_join" << std::endl;
    }

    // 关闭定时器
    struct itimerval tick;
    tick.it_interval.tv_sec = 0; // 设置 interval 为 0, 完成本次定时之后, 停止定时
    tick.it_interval.tv_usec = 0;
    setitimer(ITIMER_REAL, &tick, NULL); // 完成本次定时之后，停止定时
    usleep(0.1 * 1e6);
    std::cout << "atExitHandler: 关闭定时器2" << std::endl;

    // 重置能耗配置
    NVMLConfig.ResetPowerLimit();
    NVMLConfig.ResetSMClkLimit();
    NVMLConfig.ResetMemClkLimit();
    // 结束 NVML
    NVMLConfig.uninit();
    // 关闭输出文件
    MFGPOEOOutStream.close();

    // 释放 CUPTI 数据缓冲空间
    // for (int i = 0; i < VECTOR_MEASURE_BUFFER_SIZE; ++i) {
    //     free(vecCUPTIBufferOrigin[i]);
    // }
    CUPTIBufferManager.clear();

    std::cout << "atExitHandler: 结束" << std::endl;
}

static CUptiResult
cuptiInitialize(void) {
    CUptiResult status = CUPTI_SUCCESS;

    CUPTI_CALL(cuptiSubscribe(&GlobalConfig.subscriber, (CUpti_CallbackFunc)callbackHandler, NULL));

    // Subscribe Driver and Runtime callbacks to call cuptiFinalize in the entry/exit callback of these APIs
    CUPTI_CALL(cuptiEnableDomain(1, GlobalConfig.subscriber, CUPTI_CB_DOMAIN_RUNTIME_API));
    CUPTI_CALL(cuptiEnableDomain(1, GlobalConfig.subscriber, CUPTI_CB_DOMAIN_DRIVER_API));

    // Enable CUPTI activities
    // CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_DRIVER));
    // CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_RUNTIME));
    CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_CONCURRENT_KERNEL));

    // Register buffer callbacks
    CUPTI_CALL(cuptiActivityRegisterCallbacks(bufferRequested, bufferCompleted));

    // Optionally get and set activity attributes.
    // Attributes can be set by the CUPTI client to change behavior of the activity API.
    // Some attributes require to be set before any CUDA context is created to be effective,
    // e.g. to be applied to all device buffer allocations (see documentation).
    // CUPTI手册说 单个buffer大小 和 buffer数量上限 只能在 CUDA初始化之前 或者 context创建之前 设置
    size_t attrValue = 0, attrValueSize = sizeof(size_t);
    CUPTI_CALL(cuptiActivityGetAttribute(CUPTI_ACTIVITY_ATTR_DEVICE_BUFFER_SIZE, &attrValueSize, &attrValue));
    printf("%s = %llu B\n", "CUPTI_ACTIVITY_ATTR_DEVICE_BUFFER_SIZE", (long long unsigned)attrValue);
    // CUPTI_ACTIVITY_ATTR_DEVICE_BUFFER_SIZE = 3200000 B
    // attrValue *= 2;
    // attrValue = 8 * 1024 * 1024;
    // CUPTI_CALL(cuptiActivitySetAttribute(CUPTI_ACTIVITY_ATTR_DEVICE_BUFFER_SIZE, &attrValueSize, &attrValue));

    CUPTI_CALL(cuptiActivityGetAttribute(CUPTI_ACTIVITY_ATTR_DEVICE_BUFFER_POOL_LIMIT, &attrValueSize, &attrValue));
    printf("%s = %llu\n", "CUPTI_ACTIVITY_ATTR_DEVICE_BUFFER_POOL_LIMIT", (long long unsigned)attrValue);
    // CUPTI_ACTIVITY_ATTR_DEVICE_BUFFER_POOL_LIMIT = 250
    // attrValue *= 2;
    // CUPTI_CALL(cuptiActivitySetAttribute(CUPTI_ACTIVITY_ATTR_DEVICE_BUFFER_POOL_LIMIT, &attrValueSize, &attrValue));

    return status;
}

void CUPTIAPI
callbackHandler(void *userdata, CUpti_CallbackDomain domain, CUpti_CallbackId cbid, void *cbdata) {
    const CUpti_CallbackData *cbInfo = (CUpti_CallbackData *)cbdata;

    // Check last error
    CUPTI_CALL(cuptiGetLastError());

    // This code path is taken only when we wish to perform the CUPTI teardown
    if (GlobalConfig.detachCupti) {
        switch(domain) {
        case CUPTI_CB_DOMAIN_RUNTIME_API:
        case CUPTI_CB_DOMAIN_DRIVER_API:
            if (GlobalConfig.istimeout == 1 && cbInfo->callbackSite == CUPTI_API_EXIT) {
                // Detach CUPTI calling cuptiFinalize() API
                std::cout << "callbackHandler: cuptiFinalize()" << std::endl;
                CUPTI_CALL(cuptiFinalize());
                PTHREAD_CALL(pthread_cond_broadcast(&GlobalConfig.mutexCondition));
                // printf("pthread_cond_broadcast\n");
            }
            break;
        default:
            break;
        }
    }
}

// 周期性测量(0.1s一次) kernel 数量 以判断 GPU 是否空载
// 通过 GPU 占用率 判断 GPU 是否空载
// 空载则降频, 不再空载则恢复频率
// 持续 2s 非空载 则 退出该函数
void monitorIdle(int SMClkUpperLimit, int MemClkUpperLimit, bool isLowClk) {

    // std::cout << "monitorIdle: 进入" << std::endl;

    float Interval = 0.1; // 测量间隔
    unsigned int TimeStep = Interval * 1e6;
    float Duration = 1.5; // 使用此时间区间中的数据判断是否 idle
    float IdlePctThreshold = 0.7; // 无占用的时间大于 90% 则认为是 空载
    float IdlePct;
    int MaxCount = Duration / Interval;
    int IdleCount; // 最近 MaxCount 次测量中 idle 的数量
    nvmlReturn_t nvmlResult;
    nvmlUtilization_t currUtil; // (%)
    unsigned int currSMUtil, currMemUtil;

    // 先判断现在是否是空载
    IdleCount = MaxCount; // 初始化 idle 判断结果, 一开始认为过去全都是 idle
    // 进行 MaxCount 次测量
    for (unsigned int i = 0; i < MaxCount; ++i) {
        nvmlResult = nvmlDeviceGetUtilizationRates(NVMLConfig.nvmlDevice, &currUtil);
        if (NVML_SUCCESS != nvmlResult) {
            printf("Failed to get utilization rate: %s\n", nvmlErrorString(nvmlResult));
            exit(-1);
        }
        currSMUtil = currUtil.gpu;
        currMemUtil = currUtil.memory;
        // 更新 IdleCount
        if (currSMUtil != 0) {
            --IdleCount;
        }

        IdlePct = (float)IdleCount / MaxCount;
        // std::cout << "monitorIdle: IdlePct = " << IdlePct << std::endl;
        if (IdlePct < IdlePctThreshold) { // 如果不是空载
            std::cout << "monitorIdle: done" << std::endl;
            return;
        }
        usleep(TimeStep);
        if (GlobalConfig.terminateThread) {
            std::cout << "monitorIdle: done" << std::endl;
            return;
        }
    }
    if (isLowClk == true)
    {
        // 执行到这里肯定是空载了
        NVMLConfig.SetSMClkUpperLimit(vecSMClk[0]); // 降频
    }
    
    // 检测是否 最近有负载, 如果 有负载 则 恢复频率上限 然后 返回
    IdlePctThreshold = 0.6;
    std::vector<bool> vecIsIdle(MaxCount, true); // 初始化 idle 判断结果循环队列
    IdleCount = MaxCount; // 初始化 idle 判断结果, 一开始认为过去全都是 idle
    unsigned int count = 0;
    while (true) {
        nvmlResult = nvmlDeviceGetUtilizationRates(NVMLConfig.nvmlDevice, &currUtil);
        if (NVML_SUCCESS != nvmlResult) {
            printf("Failed to get utilization rate: %s\n", nvmlErrorString(nvmlResult));
            exit(-1);
        }
        currSMUtil = currUtil.gpu;
        currMemUtil = currUtil.memory;
        unsigned int index = count % MaxCount; // 计算 循环队列 index
        // 更新 idle 判断结果
        IdleCount -= vecIsIdle[index]; // 丢弃旧数据
        vecIsIdle[index] = (currSMUtil == 0); // 新数据更新到队列
        IdleCount += vecIsIdle[index]; // 加上新数据

        IdlePct = (float)IdleCount / MaxCount;
        // std::cout << "monitorIdle: IdlePct = " << IdlePct << std::endl;

        if (IdlePct < IdlePctThreshold) { // 如果不再是 idle
            break;
        }

        ++count;
        usleep(TimeStep);
        if (GlobalConfig.terminateThread) {
            // 恢复频率配置
            if (isLowClk == true)
            {
                if (SMClkUpperLimit > 0){
                    NVMLConfig.SetSMClkUpperLimit(SMClkUpperLimit);
                } else {
                    NVMLConfig.ResetSMClkLimit();
                }
                #if defined(SUPPORT_SET_MEMORY_CLOCK)
                if (MemClkUpperLimit > 0) {
                    NVMLConfig.SetMemClkUpperLimit(MemClkUpperLimit);
                } else {
                    NVMLConfig.ResetMemClkLimit();
                }
                #endif
            }
            std::cout << "monitorIdle: TimeStep = " << std::dec << TimeStep << std::endl;
            std::cout << "monitorIdle: count = " << std::dec << count << std::endl;
            std::cout << "monitorIdle: done" << std::endl;
            sleep(2);
            return;
        }
    }

    // 恢复频率配置
    if (isLowClk == true)
    {
        if (SMClkUpperLimit > 0){
            NVMLConfig.SetSMClkUpperLimit(SMClkUpperLimit);
        } else {
            NVMLConfig.ResetSMClkLimit();
        }
        #if defined(SUPPORT_SET_MEMORY_CLOCK)
        if (MemClkUpperLimit > 0) {
            NVMLConfig.SetMemClkUpperLimit(MemClkUpperLimit);
        } else {
            NVMLConfig.ResetMemClkLimit();
        }
        #endif
    }
    std::cout << "monitorIdle: TimeStep = " << std::dec << TimeStep << std::endl;
    std::cout << "monitorIdle: count = " << std::dec << count << std::endl;
    std::cout << "monitorIdle: done" << std::endl;
    sleep(1);
}

void measureData(int index, float interval, bool cupti_flag, unsigned int SMClkUpperLimit, unsigned int MemClkUpperLimit) {

    // 要保证测量数据的正确性, 保证数据都在同一频率配置下测得的

    std::cout << "measureData: 进入" << std::endl;
    std::cout << "measureData: index = " << std::dec << index << std::endl;
    std::cout << "measureData: interval = " << std::fixed << std::setprecision(3) << interval << std::endl;
    std::cout << "measureData: cupti_flag = " << cupti_flag << std::endl;

    float tmpSampleInterval = (float)SAMPLE_INTERVAL / 1000.0f; // 毫秒ms 转为 秒s

    float minMeasureDuration = 2.5 * tmpSampleInterval;
    interval = std::max((float)interval, minMeasureDuration); // 限制测量时长下限, 防止出现测量时长为 0 的情况
    MFGPOEOOutStream << "\nmeasureData: MeasureWindowDuration = " << std::fixed << std::setprecision(3) << interval << std::endl;

    double MeasureEndTimeStamp;
    struct timeval EndTimeStamp;
    gettimeofday(&EndTimeStamp, NULL);
    MeasureEndTimeStamp = (double)EndTimeStamp.tv_sec + (double)EndTimeStamp.tv_usec * 1e-6 - GlobalConfig.StartTimeStamp;
    // 设置全局数据缓冲的 index
    unsigned int tmpIndex = (index + 1) % vecMeasureData.size();
    unsigned int measure_count = 0;
    
    // 如果没测到数据 就 再测量一次
    while (!GlobalConfig.terminateThread) {

        measure_count++;
        if (measure_count == 1) {
            global_data_index = index; // 设置测量缓冲全局index
            vecMeasureData[index].clear(); // 清空测量数据缓冲
        } else {
            global_data_index = tmpIndex; // 设置测量缓冲全局index
            vecMeasureData[tmpIndex].clear(); // 清空测量数据缓冲
        }

        // 先等待应用不是空载
        monitorIdle(SMClkUpperLimit, MemClkUpperLimit, true);
        // Check the condition again
        CHECK_TERMINATION_RETURN;
        
        // 注册时钟信号
        // tv_usec 的取值范围 [0, 999999]
        long tmp_tv_sec = std::floor(tmpSampleInterval);
        long tmp_tv_usec = ((unsigned int)(tmpSampleInterval * 1e6) % (unsigned int)1e6);
        // long tmp_tv_sec = SAMPLE_INTERVAL * 1000 / (unsigned int)1e9;
        // long tmp_tv_usec = (SAMPLE_INTERVAL * 1000 % (unsigned int)1e9);
        signal(SIGALRM, AlarmSampler);
        struct itimerval tick;
        tick.it_value.tv_sec = 0; // 0秒钟后将启动定时器
        tick.it_value.tv_usec = 1;
        // tick.it_interval.tv_sec = 0; // 定时器启动后，每隔 100ms 将执行相应的函数
        // tick.it_interval.tv_usec = SAMPLE_INTERVAL * 1000; // 100ms
        tick.it_interval.tv_sec = tmp_tv_sec; // 定时器启动后，每隔 100ms 将执行相应的函数
        tick.it_interval.tv_usec = tmp_tv_usec; // 100ms

        uint64_t start = 0, end = 0;
        struct timeval StartTimeVal, EndTimeVal;
        double tmpTimeStamp;
        double MeasureDuration;

        if (cupti_flag) {
            // 启动 CUPTI 测量
            GlobalConfig.detachCupti = 0;
            GlobalConfig.istimeout = 0;
            CUPTI_CALL(cuptiInitialize());
            CUPTI_CALL(cuptiGetTimestamp(&start));
            GlobalConfig.tracingEnabled = 1;
            std::cout << "measureData: 启动 CUPTI 测量" << std::endl;
        } else {
            gettimeofday(&StartTimeVal, NULL);
        }
        GlobalConfig.isMeasuring = 1;
        
        // Check the condition again
        if (GlobalConfig.terminateThread) {
            if (cupti_flag) {
                GlobalConfig.detachCupti = 1;
                GlobalConfig.istimeout = 1;
                // GlobalConfig.tracingEnabled = 1;
            }
            // GlobalConfig.isMeasuring = 1;
            return;
        }

        // 启动 NVML 测量
        std::cout << "measureData: 启动 NVML 测量" << std::endl;
        setitimer(ITIMER_REAL, &tick, NULL);

        // 每隔 0.01s 检查下是否要提前结束
        uint64_t TimeStep = 0.01 * 1e6;
        uint64_t RemainingTime = interval * 1e6; // 剩余时间 us
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
            if (GlobalConfig.terminateThread) {
                if (cupti_flag) {
                    GlobalConfig.detachCupti = 1;
                    GlobalConfig.istimeout = 1;
                    // GlobalConfig.tracingEnabled = 1;
                }
                // GlobalConfig.isMeasuring = 1;
                
                tick.it_interval.tv_sec = 0; // 设置 interval 为 0, 完成本次定时之后, 停止定时
                tick.it_interval.tv_usec = 0;
                setitimer(ITIMER_REAL, &tick, NULL); // 完成本次定时之后，停止定时
                std::cout << "measureData: 停止 NVML 测量" << std::endl;

                std::cout << "measureData: 延时提前结束" << std::endl;
                return;
            }
        }
        std::cout << "measureData: 延时结束" << std::endl;
        if (cupti_flag) {
            GlobalConfig.istimeout = 1;
        }

        // 停止 NVML 测量
        // 结束采样
        // struct itimerval tick;
        getitimer(ITIMER_REAL, &tick); // 获得本次定时的剩余时间
        tick.it_interval.tv_sec = 0; // 设置 interval 为 0, 完成本次定时之后, 停止定时
        tick.it_interval.tv_usec = 0;
        setitimer(ITIMER_REAL, &tick, NULL); // 完成本次定时之后，停止定时
        std::cout << "measureData: 停止 NVML 测量" << std::endl;

        // Check the condition again
        if (GlobalConfig.terminateThread) {
            if (cupti_flag) {
                GlobalConfig.detachCupti = 1;
                GlobalConfig.istimeout = 1;
                // GlobalConfig.tracingEnabled = 1;
            }
            // GlobalConfig.isMeasuring = 1;
            std::cout << "measureData: 提前结束" << std::endl;
            return;
        }
        
        if (cupti_flag) {
            std::cout << "measureData: 开始停止 CUPTI 测量" << std::endl;
            // 读取 CUPTI 测量数据
            // Force flush
            if (GlobalConfig.tracingEnabled) {
                std::cout << "measureData: 开始 flush" << std::endl;
                CUPTI_CALL(cuptiActivityFlushAll(1));
                // CUPTI_CALL(cuptiActivityFlushAll(0));
            }
            CUPTI_CALL(cuptiGetTimestamp(&end));
            gettimeofday(&EndTimeVal, NULL);
            std::cout << "measureData: flush" << std::endl;

            // 停止 CUPTI 测量
            GlobalConfig.detachCupti = 1;
            if (GlobalConfig.tracingEnabled) {
                // Lock and wait for callbackHandler() to perform CUPTI teardown
                PTHREAD_CALL(pthread_mutex_lock(&GlobalConfig.mutexFinalize));
                PTHREAD_CALL(pthread_cond_wait(&GlobalConfig.mutexCondition, &GlobalConfig.mutexFinalize));
                PTHREAD_CALL(pthread_mutex_unlock(&GlobalConfig.mutexFinalize));
            }
            std::cout << "measureData: 等待结束" << std::endl;

            GlobalConfig.tracingEnabled = 0;
            GlobalConfig.istimeout = 0;
            GlobalConfig.detachCupti = 0;
            GlobalConfig.subscriber = 0;
            std::cout << "measureData: 停止 CUPTI 测量" << std::endl;
        } else {
            gettimeofday(&EndTimeVal, NULL);
        }
        gettimeofday(&EndTimeStamp, NULL);
        MeasureEndTimeStamp = (double)EndTimeStamp.tv_sec + (double)EndTimeStamp.tv_usec * 1e-6 - GlobalConfig.StartTimeStamp;

        // 延时 保证 NVML 测量数据的完整性
        getitimer(ITIMER_REAL, &tick); // 获得本次定时的剩余时间
        tick.it_value.tv_usec += 5000; // 5ms
        usleep(tick.it_value.tv_usec);
        
        GlobalConfig.isMeasuring = 0; // 至此 NVML 和 CUPTI 测量才都结束

        // Check the condition again
        CHECK_TERMINATION_RETURN;

        // 计算测量时长
        if (cupti_flag) {
            MeasureDuration = (double)(end - start) * 1e-9;
        } else {
            MeasureDuration = ((double)EndTimeVal.tv_sec - StartTimeVal.tv_sec) + ((double)EndTimeVal.tv_usec - StartTimeVal.tv_usec) * 1e-6;
        }

        std::cout << "measureData: 解析数据" << std::endl;
        // 进行数据处理, 计算性能指标 加权运行时间
        if (measure_count == 1) {
            vecMeasureData[index].Analysis(MeasureEndTimeStamp, SAMPLE_INTERVAL, MeasureDuration, SMClkUpperLimit, MemClkUpperLimit, cupti_flag);
        } else {
            vecMeasureData[tmpIndex].Analysis(MeasureEndTimeStamp, SAMPLE_INTERVAL, MeasureDuration, SMClkUpperLimit, MemClkUpperLimit, cupti_flag);
            // 如果多次测量 则 融合数据, 这里 discount = 1.0 表示要强制融合
            vecMeasureData[index].Merge(vecMeasureData[tmpIndex], 1.0, false, GlobalConfig.ExeTimeRadius, GlobalConfig.GapTimeRadius); // 融合测量数据
        }
        std::cout << "measureData: 解析完成" << std::endl;

        std::cout << "measureData: MeasureDuration = " << std::fixed << std::setprecision(1) << vecMeasureData[index].MeasureDuration << std::endl;
        MFGPOEOOutStream << "MeasureWindowDuration = " << std::fixed << std::setprecision(1) << vecMeasureData[index].MeasureDuration << std::endl;

        // 如果 GPU SM 实际占用时间过短 则 继续测量
        if (vecMeasureData[index].MeasureDuration / interval < 0.9) {
            interval -= vecMeasureData[index].MeasureDuration;
            interval *= 1.1;
            if (interval > minMeasureDuration) {
                MFGPOEOOutStream << "new interval = " << std::fixed << std::setprecision(3) << interval << std::endl;
            } else {
                break;
            }
        } else if (cupti_flag == true) {
            if (vecMeasureData[index].map2Kernel.size() == 0) {
                std::cout << "measureData: 没测到数据, 继续测量" << std::endl;
            } else {
                break; // 不再测量
            }
        } else {
            break; // 不再测量
        }
    }

    if (cupti_flag == true) // 累加 CUPTI 测量时间
    {
        sumMeasurementTime += vecMeasureData[index].MeasureDuration;
    }

    MFGPOEOOutStream << "\nmeasureData: MeasureWindowDuration = " << std::fixed << std::setprecision(3) << vecMeasureData[index].MeasureDuration << std::endl;
    MFGPOEOOutStream 
    << "measureData: avgPower: " << std::fixed << std::setprecision(1) << vecMeasureData[index].avgPower << "; " 
    << "avgSMClk: " << std::fixed << std::setprecision(1) << vecMeasureData[index].avgSMClk << "; " 
    << "avgMemClk: " << std::fixed << std::setprecision(1) << vecMeasureData[index].avgMemClk << "; " 
    << "avgGPUUtil: " << std::fixed << std::setprecision(1) << vecMeasureData[index].avgGPUUtil << "; " 
    << "avgMemUtil: " << std::fixed << std::setprecision(1) << vecMeasureData[index].avgMemUtil
    << std::endl;
    MFGPOEOOutStream << "measureData: NumKernels = " << std::dec << vecMeasureData[index].NumKernels << std::endl;
    MFGPOEOOutStream << "measureData: NumGaps = " << std::dec << vecMeasureData[index].NumGaps << std::endl;

    std::cout << "measureData: 结束" << std::endl;
    return;
}

// 测量 默认配置下的 基准运行数据: 加权运行时间, 平均功率等
void measureBaseData(int indexBase, float interval, float KernelsPerSec) {
    std::cout << "measureBaseData: 进入" << std::endl;
    NVMLConfig.ResetSMClkLimit();
    NVMLConfig.ResetMemClkLimit();
    NVMLConfig.ResetPowerLimit();
    sleep(2);
    CHECK_TERMINATION_RETURN;

    // nvmlReturn_t nvmlResult;
    // nvmlResult = nvmlDeviceGetMaxClockInfo(NVMLConfig.nvmlDevice, NVML_CLOCK_SM, &SMClkMax);
    // if (NVML_SUCCESS != nvmlResult) {
    //     printf("Failed to get SMClkMax: %s\n", nvmlErrorString(nvmlResult));
    //     exit(-1);
    // }
    // nvmlResult = nvmlDeviceGetMaxClockInfo(NVMLConfig.nvmlDevice, NVML_CLOCK_MEM, &MemClkMax);
    // if (NVML_SUCCESS != nvmlResult) {
    //     printf("Failed to get MemClkMax: %s\n", nvmlErrorString(nvmlResult));
    //     exit(-1);
    // }
    // NVML_CLOCK_GRAPHICS = 0
    // Graphics clock domain.
    // NVML_CLOCK_SM = 1
    // SM clock domain.
    // NVML_CLOCK_MEM = 2
    // MemClkMax.

    // measureData(indexBase, 1.5, true, GlobalConfig.MaxSMClk, GlobalConfig.MaxMemClk);
    // CHECK_TERMINATION_RETURN;

    measureData(indexBase, interval, true, GlobalConfig.MaxSMClk, GlobalConfig.MaxMemClk);
    CHECK_TERMINATION_RETURN;
}

void monitorStageChange(float threshold, float interval, int window_size, int SMClkUpperLimit, int MemClkUpperLimit) {

    std::cout << "monitorStageChange: 进入" << std::endl;
    sleep(2);

    // while (!GlobalConfig.terminateThread) {
    //     usleep(0.2*1e6);
    // }
    // return;

    // 测量基准数据: 采集几次数据, 求平均
    MEASURE_DATA NVMLBase;
    int begin = (global_measure_count + 1) % VECTOR_MEASURE_DATA_SIZE;
    int num = 3; // window_size
    for (int i = 0; i < num; i++)
    {
        global_measure_count++;
        int index = global_measure_count % VECTOR_MEASURE_DATA_SIZE; // 当前控制区间
        // 测量数据: NVML
        measureData(index, interval, false, SMClkUpperLimit, MemClkUpperLimit);
        CHECK_TERMINATION_RETURN;
    }
    NVMLBase.AvgNVMLData(vecMeasureData, begin, num);
    MFGPOEOOutStream 
    << "NVMLBase.avgPower: " << std::fixed << std::setprecision(1) << NVMLBase.avgPower << "; " 
    << "NVMLBase.avgSMClk: " << std::fixed << std::setprecision(1) << NVMLBase.avgSMClk << "; " 
    << "NVMLBase.avgMemClk: " << std::fixed << std::setprecision(1) << NVMLBase.avgMemClk << "; " 
    << "NVMLBase.avgGPUUtil: " << std::fixed << std::setprecision(1) << NVMLBase.avgGPUUtil << "; " 
    << "NVMLBase.avgMemUtil: " << std::fixed << std::setprecision(1) << NVMLBase.avgMemUtil
    << std::endl;

    // window_size 中 50% 的数据超过阈值 则认为不稳定了
    float UnstablePct;
    int tmp_count = -1;
    int UnstableCount = 0;
    std::vector<bool> vecUnstable(window_size, false); // 初始化 unstable 判断结果循环队列
    float SleepDuration = 0.0;
    while (!GlobalConfig.terminateThread) {
        tmp_count++;
        global_measure_count++;
        int index = global_measure_count % VECTOR_MEASURE_DATA_SIZE; // 当前控制区间
        // 测量数据: NVML
        measureData(index, interval, false, SMClkUpperLimit, MemClkUpperLimit);
        CHECK_TERMINATION_RETURN;

        // 判断新测量的数据是否是稳定的
        bool isUnStable;
        float distance = NVMLDistance(NVMLBase, vecMeasureData[index]);
        if (distance > threshold) {
            isUnStable = true;
            SleepDuration = 0.0;
        } else {
            isUnStable = false;
            if (tmp_count < window_size - 1) {
                SleepDuration = 0.0;
            } else if (SleepDuration < interval) {
                SleepDuration = interval;
            } else {
                SleepDuration *= 2;
                SleepDuration = std::min(SleepDuration, 8*interval);
            }
        }

        int index_flag = tmp_count % window_size; // 计算 循环队列 index_flag
        // 更新 unstable 判断结果
        UnstableCount -= (vecUnstable[index_flag]==true); // 丢弃旧数据
        vecUnstable[index_flag] = isUnStable; // 新数据更新到队列
        UnstableCount += (vecUnstable[index_flag]==true); // 加上新数据

        UnstablePct = (float)UnstableCount / window_size;

        if (UnstablePct > 0.5) { // 如果有 50% 的数据超过阈值 则认为不稳定了
            // 跳出
            MFGPOEOOutStream << "monitorStageChange: UnstablePct = " << std::fixed << std::setprecision(2) << UnstablePct << std::endl;
            break;
        }

        // 延时一段时间
        if (SleepDuration > 0.1) {
            std::cout << "monitorStageChange: 延时 " << std::fixed << std::setprecision(2) << SleepDuration << " s" << std::endl;
            // 每隔 0.1s 检查下是否要提前结束
            uint64_t TimeStep = 0.1 * 1e6;
            uint64_t RemainingTime = SleepDuration * 1e6; // 剩余时间 us
            while (true) {
                if (RemainingTime > TimeStep) {
                    usleep(TimeStep);
                    RemainingTime -= TimeStep;
                } else {
                    usleep(RemainingTime);
                    RemainingTime = 0;
                    break;
                }

                // Check the condition again
                if (GlobalConfig.terminateThread) {
                    std::cout << "monitorStageChange: 延时提前结束" << std::endl;
                    return;
                }
            }
            std::cout << "monitorStageChange: 延时结束" << std::endl;
        }
    }

    std::cout << "monitorStageChange: 结束" << std::endl;
}

void measureNVMLData(MEASURE_DATA& NVMLData, int index, float interval, unsigned int SMClkUpperLimit, unsigned int MemClkUpperLimit) {
    std::cout << "measureNVMLData: 进入" << std::endl;
    std::cout << "measureNVMLData: index = " << std::dec << index << std::endl;
    std::cout << "measureNVMLData: interval = " << std::fixed << std::setprecision(1) << interval << std::endl;

    float tmpSampleInterval = SAMPLE_INTERVAL / 1000;

    float minMeasureDuration = 2.5 * tmpSampleInterval;
    interval = std::max((float)interval, minMeasureDuration); // 限制测量时长下限, 防止出现测量时长为 0 的情况
    global_data_index = index;

    // Check the condition again
    CHECK_TERMINATION_RETURN;

    vecMeasureData[index].clear();
    NVMLData.clear();
    // 注册时钟信号
    // tv_usec 的取值范围 [0, 999999]
    long tmp_tv_sec = tmpSampleInterval;
    long tmp_tv_usec = ((unsigned int)(tmpSampleInterval * 1e6) % (unsigned int)1e6);
    // long tmp_tv_sec = SAMPLE_INTERVAL * 1000 / (unsigned int)1e9;
    // long tmp_tv_usec = (SAMPLE_INTERVAL * 1000 % (unsigned int)1e9);
    signal(SIGALRM, AlarmSampler);
    struct itimerval tick;
    tick.it_value.tv_sec = 0; //0秒钟后将启动定时器
    tick.it_value.tv_usec = 1;
    // tick.it_interval.tv_sec = 0; //定时器启动后，每隔 100ms 将执行相应的函数
    // tick.it_interval.tv_usec = SAMPLE_INTERVAL * 1000; // 100ms
    tick.it_interval.tv_sec = tmp_tv_sec; // 定时器启动后，每隔 100ms 将执行相应的函数
    tick.it_interval.tv_usec = tmp_tv_usec; // 100ms

    uint64_t start = 0, end = 0;
    struct timeval StartTimeVal, EndTimeVal;
    double tmpTimeStamp;

    gettimeofday(&StartTimeVal, NULL);
    GlobalConfig.isMeasuring = 1;

    // Check the condition again
    CHECK_TERMINATION_RETURN;

    double MeasureDurationFixed = 2.5;
    double MeasureDuration;
    double MeasureDurationLeft = interval;

    // 启动 NVML 测量
    std::cout << "measureNVMLData: 启动 NVML 测量" << std::endl;
    setitimer(ITIMER_REAL, &tick, NULL);

    // 每隔 0.05s 检查下是否要提前结束
    uint64_t TimeStep = 0.05 * 1e6;
    uint64_t RemainingTime = interval * 1e6; // 剩余时间 us
    while (true) {
        // std::cout << "measureNVMLData: RemainingTime = " << std::dec << RemainingTime << std::endl;
        if (RemainingTime > TimeStep) {
            usleep(TimeStep);
            RemainingTime -= TimeStep;
        } else {
            usleep(RemainingTime);
            RemainingTime = 0;
            break;
        }

        // Check the condition again
        if (GlobalConfig.terminateThread) {                
            tick.it_interval.tv_sec = 0; // 设置 interval 为 0, 完成本次定时之后, 停止定时
            tick.it_interval.tv_usec = 0;
            setitimer(ITIMER_REAL, &tick, NULL); // 完成本次定时之后，停止定时
            std::cout << "measureNVMLData: 停止 NVML 测量" << std::endl;
            std::cout << "measureNVMLData: 延时提前结束" << std::endl;
            return;
        }
    }
    std::cout << "measureNVMLData: 延时结束" << std::endl;

    // 停止 NVML 测量
    // 结束采样
    // struct itimerval tick;
    getitimer(ITIMER_REAL, &tick); // 获得本次定时的剩余时间
    tick.it_interval.tv_sec = 0; // 设置 interval 为 0, 完成本次定时之后, 停止定时
    tick.it_interval.tv_usec = 0;
    setitimer(ITIMER_REAL, &tick, NULL); // 完成本次定时之后，停止定时
    std::cout << "measureNVMLData: 停止 NVML 测量" << std::endl;

    // Check the condition again
    CHECK_TERMINATION_RETURN;

    gettimeofday(&EndTimeVal, NULL);

    // 延时 保证 NVML 测量数据的完整性
    getitimer(ITIMER_REAL, &tick); // 获得本次定时的剩余时间
    tick.it_value.tv_usec += 5000; // 5ms
    usleep(tick.it_value.tv_usec);
    
    GlobalConfig.isMeasuring = 0; // 至此 NVML 和 CUPTI 测量才都结束

    // Check the condition again
    CHECK_TERMINATION_RETURN;

    // 计算测量时长
    MeasureDuration = ((double)EndTimeVal.tv_sec - StartTimeVal.tv_sec) + ((double)EndTimeVal.tv_usec - StartTimeVal.tv_usec) * 1e-6;
    tmpTimeStamp = (double)EndTimeVal.tv_sec + (double)EndTimeVal.tv_usec * 1e-6 - GlobalConfig.StartTimeStamp;

    std::cout << "measureNVMLData: 解析数据" << std::endl;
    // 进行数据处理, 计算性能指标 加权运行时间
    vecMeasureData[index].Analysis(tmpTimeStamp, SAMPLE_INTERVAL, MeasureDuration, SMClkUpperLimit, MemClkUpperLimit, false);
    std::cout << "measureNVMLData: 解析完成" << std::endl;
    NVMLData.Energy = vecMeasureData[index].Energy;
    NVMLData.Energy = vecMeasureData[index].Energy;
    NVMLData.avgPower = vecMeasureData[index].avgPower;
    NVMLData.avgSMClk = vecMeasureData[index].avgSMClk;
    NVMLData.avgMemClk = vecMeasureData[index].avgMemClk;
    NVMLData.avgGPUUtil = vecMeasureData[index].avgGPUUtil;
    NVMLData.avgMemUtil = vecMeasureData[index].avgMemUtil;

    std::cout << "measureNVMLData: MeasureDuration = " << std::fixed << std::setprecision(1) << MeasureDuration << std::endl;

    std::cout << "measureNVMLData: 结束" << std::endl;
}

float DetermineMeasurementDuration() {
    float MeasurementDuration = GlobalConfig.MeasureWindowDuration;
    float prevMeasurementDuration = MeasurementDuration;

    NVMLConfig.ResetSMClkLimit();
    NVMLConfig.ResetMemClkLimit();
    NVMLConfig.ResetPowerLimit();
    sleep(2);
    CHECK_TERMINATION_RETURN MeasurementDuration;

    float EVDistance = 1.0;
    int Count = 0;
    float tmpDiscount = 1.0;
    bool isIncrementalMeasurement = false;
    float IncrementalMeasurementDuration = -1.0;

    int IndexBase;
    BaseData.clear();
    CUPTIBufferCount = -1; // 全局变量
    global_measure_count = -1;
    global_measure_count++;
    IndexBase = global_measure_count % VECTOR_MEASURE_DATA_SIZE;

    printTimeStamp();
    MFGPOEOOutStream << "\nMeasurementDuration = " << std::fixed << std::setprecision(1) << MeasurementDuration << std::endl;
    MFGPOEOOutStream << "measureBaseData..." << std::endl;
    std::cout << "measureBaseData..." << std::endl;
    measureData(IndexBase, MeasurementDuration, false, GlobalConfig.MaxSMClk, GlobalConfig.MaxMemClk);
    CHECK_TERMINATION_RETURN MeasurementDuration; // 如果停止标志为 true 直接跳出
    BaseData = vecMeasureData[IndexBase]; // 初始化 Base 数据
    isIncrementalMeasurement = false;
    
    do {
        float tmpMeasurementDuration;
        if (isIncrementalMeasurement == true && IncrementalMeasurementDuration > 0.0)
        {
            tmpMeasurementDuration = IncrementalMeasurementDuration;
        } else
        {
            tmpMeasurementDuration = MeasurementDuration;
        }

        printTimeStamp();
        MFGPOEOOutStream << "\nMeasurementDuration = " << std::fixed << std::setprecision(1) << tmpMeasurementDuration << std::endl;
        MFGPOEOOutStream << "measureBaseData..." << std::endl;
        std::cout << "measureBaseData..." << std::endl;
        global_measure_count++;
        IndexBase = global_measure_count % VECTOR_MEASURE_DATA_SIZE;
        measureData(IndexBase, tmpMeasurementDuration, false, GlobalConfig.MaxSMClk, GlobalConfig.MaxMemClk);
        CHECK_TERMINATION_BREAK;

        // 增量式测量: 衰减系数 == 1.0
        if (isIncrementalMeasurement == true)
        {
            tmpDiscount = 1.0;
            MFGPOEOOutStream << "MeasurementDuration IncrementalMeasurementDuration = " << std::fixed << std::setprecision(1) << IncrementalMeasurementDuration << std::endl;
            isIncrementalMeasurement = false;
        } else // 全区间测量: 衰减系数 可以<1.0, 例如0.8/0.7
        {
            Count++;
            MFGPOEOOutStream << "MeasurementDuration Count = " << std::dec << Count << std::endl;

            EVDistance = std::abs(BaseData.avgGPUUtil-vecMeasureData[IndexBase].avgGPUUtil)/(BaseData.avgGPUUtil+vecMeasureData[IndexBase].avgGPUUtil);
            float tmpGPUUtil = (BaseData.avgGPUUtil + vecMeasureData[IndexBase].avgGPUUtil) / 2;
            float tmpMemUtil = (BaseData.avgMemUtil + vecMeasureData[IndexBase].avgMemUtil) / 2;
            float deltaGPUUtil = std::abs(BaseData.avgGPUUtil-vecMeasureData[IndexBase].avgGPUUtil);
            float deltaMemUtil = std::abs(BaseData.avgMemUtil-vecMeasureData[IndexBase].avgMemUtil);
            MFGPOEOOutStream << "MeasurementDuration EVDistance = " << std::fixed << std::setprecision(4) << EVDistance << std::endl;
            MFGPOEOOutStream << "MeasurementDuration deltaGPUUtil = " << std::fixed << std::setprecision(4) << deltaGPUUtil << std::endl;
            MFGPOEOOutStream << "MeasurementDuration deltaMemUtil = " << std::fixed << std::setprecision(4) << deltaMemUtil << std::endl;

            prevMeasurementDuration = MeasurementDuration;
            if (EVDistance > 0.18) {
                Count = 0;
                tmpDiscount = 0.0;
                MFGPOEOOutStream << "DetermineMeasurementDuration: Abandon old data" << std::endl;

            } else if ((EVDistance > 0.05 && tmpGPUUtil < 25)
                || (EVDistance > 0.04 && 25 <= tmpGPUUtil && tmpGPUUtil < 82)
                || (EVDistance > 0.04 && tmpGPUUtil >= 82)
                || (deltaGPUUtil > 2.4 && tmpGPUUtil < 50)
                || (deltaGPUUtil > 3.0 && tmpGPUUtil < 60)
                || (deltaGPUUtil > 7.0 && tmpGPUUtil >= 60)
                // || deltaMemUtil > 8.0
                // || (deltaMemUtil > 1.2 && tmpGPUUtil < 40)
                // || (deltaMemUtil > 3.0 && tmpGPUUtil >= 40)
            ){
                // MeasurementDuration *= (Count+1);
                MeasurementDuration += 4.0;
                Count = 0;
                MeasurementDuration = std::min((float)GlobalConfig.WindowDurationMax, (float)MeasurementDuration);
                IncrementalMeasurementDuration = MeasurementDuration - prevMeasurementDuration;
                MFGPOEOOutStream << "MeasurementDuration Increase: " << std::fixed << std::setprecision(1) << prevMeasurementDuration << " s --> " << MeasurementDuration << " s" << std::endl;
                if (IncrementalMeasurementDuration > 0.0)
                {
                    isIncrementalMeasurement = true;
                }
                if (EVDistance > 0.15)
                {
                    tmpDiscount = 0.1;
                } else {
                    tmpDiscount = 0.8;
                }
            } else
            {
                isIncrementalMeasurement = false;
                tmpDiscount = 0.8;
            }
        }

        BaseData.Merge(vecMeasureData[IndexBase], tmpDiscount, false, GlobalConfig.ExeTimeRadius, GlobalConfig.GapTimeRadius); // 融合 Base 历史数据

    } while (!GlobalConfig.terminateThread && MeasurementDuration < GlobalConfig.WindowDurationMax
        && ((Count < 3 && MeasurementDuration <= 10) || (Count < 1 && MeasurementDuration > 10))
    );
    MFGPOEOOutStream << "DetermineMeasurementDuration: MeasurementDuration = " << std::fixed << std::setprecision(1) << MeasurementDuration << " s" << std::endl;
    
    return MeasurementDuration;
}

void *AdaptiveWindowEnergyOptimization(void *arg)
{
    CHECK_TERMINATION_RETURN NULL; // 如果停止标志为 true 直接跳出

    // 初始化: 
    float MeasureWindowDuration = GlobalConfig.MeasureWindowDuration; // 测量窗口长度 (s)
    float prevMeasureWindowDuration = MeasureWindowDuration; // 上次 测量窗口长度 (s)
    float Pct_Inc = GlobalConfig.Pct_Inc; // < Pct_Inc 时 尝试增大窗口长度
    float Pct_Ctrl = GlobalConfig.Pct_Ctrl; // > Pct_Ctrl 时 进行 PID 控制, 更新最优能耗配置
    int BaseUpdateCycle = 2; // 基准更新频次
    // int BaseUpdateCycle = GlobalConfig.BaseUpdateCycle; // 基准更新频次
    int NumOptConfig = GlobalConfig.NumOptConfig; // 确定稳定所需的最近最优配置数量
    int NumCtrlPctErr = NumOptConfig;
    unsigned int NumKernels = 0;

    int ResultQueLen = NumOptConfig + 3;
    CIRCULAR_QUEUE<unsigned int> queOptSMClkGear(ResultQueLen, 0);
    CIRCULAR_QUEUE<double> queCtrlPctErr(ResultQueLen, 1.0);
    CIRCULAR_QUEUE<double> queOptObj(ResultQueLen, 10.0);

    double PerfIndex = GlobalConfig.objective;
    double Objective = GlobalConfig.objective;
    float CoveredExeTimePct = 0.0; // 数据库覆盖到的 kernel数据 的百分比
    float MatchedExeTimePct = 0.0; // 在数据库中匹配到的 kernel数据 的百分比
    float MatchedGapTimePct = 0.0;  // 在数据库中匹配到的 Gap数据 的百分比
    float ExeIndex = 0.0, GapIndex = 0.0;
    float avgMatchedGapTimePct = 0.0;  // 在数据库中匹配到的 Gap数据 的百分比
    unsigned int MatchedStreams = 0; // 在数据库中匹配到的 kernel数据 所在的 stream 的数量
    float UnMatchedStreamExeTimePct = 0.0; // CtrlData 中没有匹配到的 stream 的 ExeTime 占 CtrlData 总 ExeTime 的比例
    sumMeasurementTime = 0.0;

    OptSMClkGear = std::min((int)std::ceil(0.90 * vecSMClk.size()), (int)vecSMClk.size()-1);
    OptMemClkGear = std::min((int)std::ceil(0.95 * vecMemClk.size()), (int)vecMemClk.size()-1);

    int IndexCtrl, IndexBase;
    struct timeval StartTimeVal, EndTimeVal;

    PID_CTRL PIDController(CTRL_CONFIG_TYPE::Clk, GlobalConfig.kp, GlobalConfig.ki, GlobalConfig.kd, 0, vecSMClk.size() - 1, ResultQueLen);

    for (size_t i = 0; i < 30; i++)
    {
        monitorIdle(-1, -1, false);
    }
    // sleep(8);
    // sleep(16); // 延时一段时间 避开一开始的不稳定阶段
    // sleep(24); // 延时一段时间 避开一开始的不稳定阶段
    // sleep(32);
    // sleep(50);
    srand(time(0));

    MeasureWindowDuration = DetermineMeasurementDuration();

    BaseData.clear();
    CUPTIBufferCount = -1; // 全局变量
    global_measure_count = -1;
    global_measure_count++;
    IndexBase = global_measure_count % VECTOR_MEASURE_DATA_SIZE;

    printTimeStamp();
    MFGPOEOOutStream << "\nMeasureWindowDuration = " << std::fixed << std::setprecision(1) << 4 << std::endl;
    MFGPOEOOutStream << "measureBaseData..." << std::endl;
    std::cout << "measureBaseData..." << std::endl;
    measureBaseData(IndexBase, 4, 0.0); // wfr 20230422 首次测量的数据会很异常, 因此先 warm-up 一次
    CHECK_TERMINATION_RETURN NULL; // 如果停止标志为 true 直接跳出
    sleep(2);

    printTimeStamp();
    MFGPOEOOutStream << "\nMeasureWindowDuration = " << std::fixed << std::setprecision(1) << MeasureWindowDuration << std::endl;
    MFGPOEOOutStream << "measureBaseData..." << std::endl;
    std::cout << "measureBaseData..." << std::endl;
    measureBaseData(IndexBase, MeasureWindowDuration, 0.0);
    CHECK_TERMINATION_RETURN NULL; // 如果停止标志为 true 直接跳出
    BaseData = vecMeasureData[IndexBase]; // 初始化 Base 数据
    NumKernels += vecMeasureData[IndexBase].NumKernels;
    sleep(2);

    CoveredExeTimePct = 1.0;
    MatchedExeTimePct = 1.0;
    MatchedGapTimePct = 1.0;
    PerfIndex = 0.0;
    float prevMatchedExeTimePct = 0.0;
    float deltaExeTimePct = 0.0;
    float sumDeltaExeTimePct = 0.0;
    float tmpDiscount = 1.0;
    float tmpSleepTime = 0.0;
    int ViolateCount = 1;
    do {
        usleep(tmpSleepTime*1e6);

        printTimeStamp();
        MFGPOEOOutStream << "\nMeasureWindowDuration = " << std::fixed << std::setprecision(1) << MeasureWindowDuration << std::endl;
        MFGPOEOOutStream << "measureBaseData..." << std::endl;
        std::cout << "measureBaseData..." << std::endl;
        global_measure_count++;
        IndexBase = global_measure_count % VECTOR_MEASURE_DATA_SIZE;
        // measureBaseData(IndexBase, MeasureWindowDuration, BaseData.KernelsPerSec);
        measureBaseData(IndexBase, MeasureWindowDuration, 0.0);
        NumKernels += vecMeasureData[IndexBase].NumKernels;
        CHECK_TERMINATION_BREAK;

        MFGPOEOOutStream << "deltaExeTimePct: " << std::fixed << std::setprecision(2) << deltaExeTimePct * 100 << " %" << std::endl;

        PerfIndex = AnalysisKernelData(CoveredExeTimePct, MatchedExeTimePct, MatchedGapTimePct, ExeIndex, GapIndex, MatchedStreams, UnMatchedStreamExeTimePct, false, BaseData, vecMeasureData[IndexBase], GlobalConfig.ExeTimeRadius, GlobalConfig.GapTimeRadius, GlobalConfig.OptObj, GlobalConfig.objective);
        CHECK_TERMINATION_BREAK;
        
        // wfr 20230112 如果匹配度增加不明显 就 尝试调整测量区间相位
        if (MatchedExeTimePct - prevMatchedExeTimePct < (float)10/100)
        {
            tmpSleepTime = 0.3 * MeasureWindowDuration;
        } else
        {
            // tmpSleepTime = 0.0;
            tmpSleepTime = 0.0 + (float)(rand()/RAND_MAX)*(0.2);
            tmpSleepTime *= MeasureWindowDuration;
        }
        prevMatchedExeTimePct = MatchedExeTimePct;

        prevMeasureWindowDuration = MeasureWindowDuration;
        if (CoveredExeTimePct < Pct_Inc || MatchedExeTimePct < Pct_Ctrl // 当覆盖度 或 匹配度都不足时, 增大测量区间时长
            || std::abs(PerfIndex) > 0.04 // 当性能指标不为 0 时增大测量区间时长
            || std::abs(ExeIndex) > 0.08 || std::abs(GapIndex) > 0.25
            || (BaseData.avgGPUUtil > 70 && BaseData.avgMemUtil > 50 && std::abs(GapIndex) > 0.05)
            || std::abs(BaseData.avgGPUUtil-vecMeasureData[IndexBase].avgGPUUtil)/(BaseData.avgGPUUtil+vecMeasureData[IndexBase].avgGPUUtil) > 0.06
        ) {
            ViolateCount++;

            if (ViolateCount >= 3)
            {
                // MeasureWindowDuration *= 1.618;
                MeasureWindowDuration *= 2;
                MeasureWindowDuration = std::min((float)GlobalConfig.WindowDurationMax, (float)MeasureWindowDuration);
                MFGPOEOOutStream << "MeasureWindowDuration Increase (PerfIndex): " << std::fixed << std::setprecision(1) << prevMeasureWindowDuration << " s --> " << MeasureWindowDuration << " s" << std::endl;
                ViolateCount = 0;
            }

            if (std::abs(ExeIndex) < 0.02 && std::abs(GapIndex) < 0.02)
            {
                tmpDiscount = 0.9;
            } else if (std::abs(ExeIndex) < 0.08 && std::abs(GapIndex) < 0.08)
            {
                tmpDiscount = 0.7;
            } else if (std::abs(ExeIndex) < 0.12 && std::abs(GapIndex) < 0.12)
            {
                tmpDiscount = 0.6;
            } else
            {
                tmpDiscount = 0.4;
            }
            
        } else {
            tmpDiscount = 0.9; // 这里 discount = 1.0 表示要强制融合 不衰减
        }

        BaseData.Merge(vecMeasureData[IndexBase], tmpDiscount, false, GlobalConfig.ExeTimeRadius, GlobalConfig.GapTimeRadius); // 融合 Base 历史数据
        
        MFGPOEOOutStream << "MatchedStreamsPct: " << std::fixed << std::setprecision(2) << (float)MatchedStreams / BaseData.map2Kernel.size() * 100 << " %" << std::endl;
    } while (!GlobalConfig.terminateThread 
        && global_measure_count < 5
        && (
        (CoveredExeTimePct < Pct_Inc || MatchedExeTimePct < Pct_Ctrl)
        || std::abs(PerfIndex) > 0.04 // 0.015 // 默认配置下性能损失指标接近 0%, 才停止数据库初始化
        || std::abs(ExeIndex) > 0.08 || std::abs(GapIndex) > 0.15
        // || global_measure_count < 2
        // || global_measure_count < 1
        )
    );
    std::cout << "AdaptiveWindowEnergyOptimization: 数据库构建完成" << std::endl;

    MFGPOEOOutStream << "GlobalConfig.StrangePerfThreshold: " << std::fixed << std::setprecision(3) << GlobalConfig.StrangePerfThreshold << std::endl;
    MFGPOEOOutStream << "GlobalConfig.GapDiscount: " << std::fixed << std::setprecision(3) << GlobalConfig.GapDiscount << std::endl;

#if defined(SUPPORT_SET_MEMORY_CLOCK)
    // 这里进行内存频率调节
    int tmpSMClkGear = std::min((int)std::ceil(0.95 * vecSMClk.size()), (int)vecSMClk.size()-1);
    NVMLConfig.SetSMClkUpperLimit(vecSMClk[tmpSMClkGear]);
    printSMClkUpperLimit(tmpSMClkGear);
    float tmpScale = 1.0 + GlobalConfig.objective;
    tmpScale *= 1.2;
    OptMemClkGear = TuneMemClk(PerfIndex, MeasureWindowDuration, tmpSMClkGear);

    // NVMLConfig.SetSMClkUpperLimit(1290);
    // NVMLConfig.SetMemClkUpperLimit(5001);
    // while (!GlobalConfig.terminateThread) {
    //     sleep(1);
    // }
    // return NULL;
    // 根据 OptMemClkGear 设置 OptSMClkGear 初值
    OptSMClkGear = std::min((int)std::ceil(0.95 * vecSMClk.size()), (int)vecSMClk.size()-1);
    if (OptMemClkGear <= 0.5 * vecMemClk.size() && BaseData.avgGPUUtil < 95)
    {   // wfr 20221231 依靠经验重选 OptSMClkGear 初值
        if (PerfIndex < 0.60 * GlobalConfig.objective)
        {
            if (BaseData.avgMemUtil < 20)
            {
                OptSMClkGear = std::min((int)std::round(0.65 * vecSMClk.size()), (int)vecSMClk.size()-1);
            } else
            {
                OptSMClkGear = std::min((int)std::round(0.85 * vecSMClk.size()), (int)vecSMClk.size()-1);
            }

        } else if (PerfIndex < 0.75 * GlobalConfig.objective)
        {
            if (BaseData.avgMemUtil < 20)
            {
                OptSMClkGear = std::min((int)std::round(0.7 * vecSMClk.size()), (int)vecSMClk.size()-1);
            } else
            {
                OptSMClkGear = std::min((int)std::round(0.80 * vecSMClk.size()), (int)vecSMClk.size()-1);
            }
            
        } else if (PerfIndex < 0.90 * GlobalConfig.objective)
        {
            if (BaseData.avgMemUtil < 20)
            {
                OptSMClkGear = std::min((int)std::round(0.7 * vecSMClk.size()), (int)vecSMClk.size()-1);
            } else
            {
                OptSMClkGear = std::min((int)std::round(0.80 * vecSMClk.size()), (int)vecSMClk.size()-1);
            }
        } else {
            OptSMClkGear = std::min((int)std::round(0.90 * vecSMClk.size()), (int)vecSMClk.size()-1);
        }
    }

    MFGPOEOOutStream << "GlobalConfig.StrangePerfThreshold: " << std::fixed << std::setprecision(3) << GlobalConfig.StrangePerfThreshold << std::endl;
    MFGPOEOOutStream << "GlobalConfig.GapDiscount: " << std::fixed << std::setprecision(3) << GlobalConfig.GapDiscount << std::endl;
#endif

    float minDistance = vecSMClk.back();
    int defaultGear;
    for (int i = vecSMClk.size() - 1; i >= 0; i--)
    {
        float tmpDistance = std::abs((float)vecSMClk[i] - (float)BaseData.avgSMClk);
        if (tmpDistance < minDistance)
        {
            minDistance = tmpDistance;
            defaultGear = i;
        } else
        {
            break;
        }
    }
    OptSMClkGear = std::min(OptSMClkGear, defaultGear);

    queOptSMClkGear.push(OptSMClkGear);

    int ctrl_count = 0;
    // bool flag_measure_base = false;
    bool flag_measure_ctrl = true;
    // bool flag_try_duration = false;
    bool base_data_updated = true;
    bool ctrl_data_updated = false;
    CoveredExeTimePct = 1.0;
    MatchedExeTimePct = 1.0;
    UnMatchedStreamExeTimePct = 0.0;
    PerfIndex = 0.0;
    std::cout << "AdaptiveWindowEnergyOptimization: before while" << std::endl;
    while (!GlobalConfig.terminateThread) {

        // 通过 衰减系数 来控制数据库更新
        if ((ctrl_count != 0 && ctrl_count%BaseUpdateCycle == 0)
            || CoveredExeTimePct < Pct_Inc 
            || MatchedExeTimePct < Pct_Ctrl 
            || UnMatchedStreamExeTimePct > 0.1
            || base_data_updated == false
        ) {
            MFGPOEOOutStream << "\nmeasureBaseData..." << std::endl;
            std::cout << "\nmeasureBaseData..." << std::endl;
            // if (BaseUpdateCycle == 1) {
            //     BaseUpdateCycle = std::min(GlobalConfig.BaseUpdateCycle, BaseUpdateCycle+3);
            // } else {
            //     BaseUpdateCycle = std::min(GlobalConfig.BaseUpdateCycle, BaseUpdateCycle+2);
            // }
            if (BaseUpdateCycle != GlobalConfig.BaseUpdateCycle)
            {
                BaseUpdateCycle = GlobalConfig.BaseUpdateCycle;
            }
            
            global_measure_count++;
            IndexBase = global_measure_count % VECTOR_MEASURE_DATA_SIZE;
            // measureBaseData(IndexBase, MeasureWindowDuration, BaseData.KernelsPerSec);
            measureBaseData(IndexBase, MeasureWindowDuration, 0.0);
            NumKernels += vecMeasureData[IndexBase].NumKernels;
            CHECK_TERMINATION_BREAK;

            float tmpPerfIndex = AnalysisKernelData(CoveredExeTimePct, MatchedExeTimePct, MatchedGapTimePct, ExeIndex, GapIndex, MatchedStreams, UnMatchedStreamExeTimePct, false, BaseData, vecMeasureData[IndexBase], GlobalConfig.ExeTimeRadius, GlobalConfig.GapTimeRadius, GlobalConfig.OptObj, GlobalConfig.objective);
            CHECK_TERMINATION_BREAK;

            bool isRelative = true;
            if (std::abs(tmpPerfIndex) < 0.02)
            {
                tmpDiscount = 0.8;
                isRelative = false;
            } else if (std::abs(tmpPerfIndex) < 0.05)
            {
                tmpDiscount = 0.4;
                isRelative = true;
            } else if (std::abs(tmpPerfIndex) < 0.08)
            {
                tmpDiscount = 0.3;
                isRelative = true;
            } else if (std::abs(tmpPerfIndex) < 0.12)
            {
                tmpDiscount = 0.2;
                isRelative = true;
            } else
            {
                tmpDiscount = 0.2;
                isRelative = true;
            }

            BaseData.Merge(vecMeasureData[IndexBase], tmpDiscount, isRelative, GlobalConfig.ExeTimeRadius, GlobalConfig.GapTimeRadius); // 融合 Base 历史数据
            base_data_updated = true;
#if defined(SUPPORT_SET_MEMORY_CLOCK)
            if (OptMemClkGear >= 0 && vecMemClk.size() > 0) {
                NVMLConfig.SetMemClkUpperLimit(vecMemClk[OptMemClkGear]);
            }
#endif
        }

        // 设置GPU能耗配置
        // 调整能耗配置 (核心频率上限)
        NVMLConfig.SetSMClkUpperLimit(vecSMClk[OptSMClkGear]);
        printSMClkUpperLimit(OptSMClkGear);
        sleep(1); // 延时使GPU工作稳定
        CHECK_TERMINATION_BREAK;

        if (flag_measure_ctrl == true) {
        // if (true) {
            global_measure_count++;
            IndexCtrl = global_measure_count % VECTOR_MEASURE_DATA_SIZE; // 当前控制区间
            // 测量一个窗口 (测量时间可以稍微长一点)
            printTimeStamp();
            MFGPOEOOutStream << "measureData..." << std::endl;
            std::cout << "measureData..." << std::endl;
            float tmpScale = 1.0 + PerfIndex;
            // float tmpScale = 1.0;
            tmpScale = std::max((float)1.0, tmpScale);
            tmpScale = std::min((float)(1.0+GlobalConfig.objective), tmpScale);
            // tmpScale = 1.0;
            measureData(IndexCtrl, MeasureWindowDuration * tmpScale, true, vecSMClk[OptSMClkGear], vecMemClk[OptMemClkGear]);
            NumKernels += vecMeasureData[IndexCtrl].NumKernels;
            CHECK_TERMINATION_BREAK;
            ctrl_data_updated = true;
        }

        // 处理数据 计算 同种 kernel 执行时间占比
        // 计算两次测量的 性能指标 同种 kernel 的 加权最大执行时间
        // 使用同种kernel数据, 计算基准和改配置后的 复合执行时间指标
        PerfIndex = AnalysisKernelData(CoveredExeTimePct, MatchedExeTimePct, MatchedGapTimePct, ExeIndex, GapIndex, MatchedStreams, UnMatchedStreamExeTimePct, true, BaseData, vecMeasureData[IndexCtrl], GlobalConfig.ExeTimeRadius, GlobalConfig.GapTimeRadius, GlobalConfig.OptObj, GlobalConfig.objective);
        queOptObj.push(vecMeasureData[IndexCtrl].Obj);
 
        deltaExeTimePct = std::abs(BaseData.avgExeTimePct - vecMeasureData[IndexBase].avgExeTimePct);
        MFGPOEOOutStream << "deltaExeTimePct: " << std::fixed << std::setprecision(2) << deltaExeTimePct * 100 << " %" << std::endl;

        if (MatchedExeTimePct >= 0.75 * Pct_Ctrl) {
            
            // 进行 PID 控制, 更新最优能耗配置
            // 更新控制目标
            // 这里考虑 stream 计算性能控制目标
            
            if (ctrl_count == 0) {
                if (BaseData.KernelsPerSec < 2000)
                {
                    Objective = GlobalConfig.objective - 0.001;
                } else
                {
                    Objective = GlobalConfig.objective - 0.003;
                }
                // Objective = GlobalConfig.objective - 0.004;
                PIDController.set_objective(Objective, OptSMClkGear);
                MFGPOEOOutStream << "PID controller sets PerfLossConstraint: " << std::scientific << std::setprecision(4) << PIDController.PerfLossConstraint << std::endl;
                std::cout << "PID controller sets PerfLossConstraint: " << std::scientific << std::setprecision(4) << PIDController.PerfLossConstraint << std::endl;
            }

            // 进行 PID 控制, 更新最优能耗配置
            OptSMClkGear = PIDController.control(PerfIndex, vecMeasureData[IndexCtrl].Obj, vecMeasureData[IndexCtrl].avgSMClk);

            avgMatchedGapTimePct = (avgMatchedGapTimePct * ctrl_count + MatchedGapTimePct) / (ctrl_count + 1);
            
            // OptSMClkGear = 103;
            // OptSMClkGear = 118; // {113:1905; 118:1980}
            MFGPOEOOutStream << "PID: SMClkGear = " << std::dec << OptSMClkGear << std::endl;
            std::cout << "PID: SMClkGear = " << std::dec << OptSMClkGear << std::endl;

            double tmpErr = sgn(PerfIndex - Objective) * PIDController.pct_err();
            queCtrlPctErr.push(tmpErr);
            queOptSMClkGear.push(OptSMClkGear);

            ctrl_count++;
        }

        // 判断开销较高, 放宽稳定条件
        int tmpNumOptConfig, tmpStableSMClkStepRange;
        float CtrlErr;
        gettimeofday(&EndTimeVal, NULL);
        double tmpTime = (double)EndTimeVal.tv_sec + (double)EndTimeVal.tv_usec * 1e-6 - GlobalConfig.StartTimeStamp;
        if (OptMemClkGear <= 0.5 * vecMemClk.size() || OptSMClkGear < 0.5 * vecSMClk.size()) {
            tmpNumOptConfig = 2;
            CtrlErr = 0.55;

        } else if (OptMemClkGear <= 0.5 * vecMemClk.size() || OptSMClkGear < 0.73 * vecSMClk.size()) {
            tmpNumOptConfig = 2;
            CtrlErr = 0.40;

        } else if (ctrl_count >= 6) {
            tmpNumOptConfig = 2;
            CtrlErr = 0.30;

        } else if (ctrl_count >= 5) {
            tmpNumOptConfig = 2;
            CtrlErr = 0.25;

        } else if (ctrl_count >= 4) {
            tmpNumOptConfig = 2;
            CtrlErr = 0.20;

        } else {
            tmpNumOptConfig = NumOptConfig;
            CtrlErr = 0.20;
        }
        // 稳定判定: 如果测量窗口不需要增长 且 近几次最优频率相近 则 认为稳定状态
        // 进入阶段变化监测: 通过低开销特征 监测阶段变化 以 判断是否重启优化
        if (base_data_updated == true && ctrl_data_updated == true 
            && ctrl_count >= tmpNumOptConfig && ctrl_count > 2
        ){
            // 根据给出的能耗配置 判断是否稳定; 稳定则求出近几次控制的平均能耗配置, 否则返回 0
            double avgCtrlPctErr = 1.0;
            unsigned int tmpSMClkGear = judgeConfigStability(tmpNumOptConfig, GlobalConfig.StableSMClkStepRange, ctrl_count, sumMeasurementTime, queOptSMClkGear, CtrlErr, queCtrlPctErr, queOptObj);

            // 如果 > 0 (即最优配置稳定 且 控制误差较小)
            // 就认为 稳定
            if (tmpSMClkGear > 0) {
                
                std::cout << "PID: 稳定" << std::endl;
                OptSMClkGear = tmpSMClkGear;
                NVMLConfig.SetSMClkUpperLimit(vecSMClk[OptSMClkGear]);
                printSMClkUpperLimit(OptSMClkGear);
                MFGPOEOOutStream << "PID: 稳定" << std::endl;
                MFGPOEOOutStream << "PID: OptSMClkGear: " << OptSMClkGear << std::endl;
                MFGPOEOOutStream << std::endl;

                // 监测阶段变化
                monitorStageChange(GlobalConfig.PhaseChangeThreshold, MeasureWindowDuration, GlobalConfig.PhaseWindow, vecSMClk[OptSMClkGear], vecMemClk[OptMemClkGear]);
                // monitorStageChange(GlobalConfig.PhaseChangeThreshold, MeasureWindowDuration, GlobalConfig.PhaseWindow, OptSMClkGear, OptMemClkGear);
                std::cout << "PID: 阶段变化, 需要重启优化" << std::endl;
                MFGPOEOOutStream << "PID: 阶段变化, 需要重启优化" << std::endl;
                MFGPOEOOutStream << std::endl;

                base_data_updated = false;
                ctrl_data_updated = false;
                // flag_measure_base = true;
                NVMLConfig.ResetMemClkLimit(); // 重置显存频率
                sumMeasurementTime = 0.0;
                ctrl_count = 0;
            }
        }

    }

    std::cout << "AdaptiveWindowEnergyOptimization 结束" << std::endl;

    return NULL;
}

// wfr 20221220 调整显存频率
int TuneMemClk(double& OptPerfIndex, double MeasureWindowDuration, int OptSMClkGear){
    MFGPOEOOutStream << "\nTuneMemClk: in" << std::endl;

    // if (BaseData.avgGPUUtil > 50 || BaseData.maxExeTimePct > 0.5)
    if (BaseData.avgMemUtil > 20)
    {
        MFGPOEOOutStream << "TuneMemClk: Skip memory clock tuning" << std::endl;
        MFGPOEOOutStream << "TuneMemClk: done" << std::endl;
        return vecMemClk.size() - 1;
    }
    
    int OptMemClkGear = 2;
    int IndexCtrl;
    double PerfIndex = GlobalConfig.objective;
    double OptObj = 1e15;
    OptPerfIndex = GlobalConfig.objective;
    double PerfLossConstraint = GlobalConfig.objective;

    float CoveredExeTimePct = 0.0; // 数据库覆盖到的 kernel数据 的百分比
    float MatchedExeTimePct = 0.0; // 在数据库中匹配到的 kernel数据 的百分比
    float MatchedGapTimePct = 0.0; // 在数据库中匹配到的 Gap数据 的百分比
    float ExeIndex = 0.0, GapIndex = 0.0;
    unsigned int MatchedStreams = 0;
    float UnMatchedStreamExeTimePct = 0.0; // CtrlData 中没有匹配到的 stream 的 ExeTime 占 CtrlData 总 ExeTime 的比例
    float deltaExeTimePct = 0.0;
    float deltaExeTimePctPrev = 0.0;
    float tmpFactor = 1.0; // 显存频率节能最多只能使用 90% 的节能潜力
    
    int tryMemClkGearBegin = 2;
    for (int i = tryMemClkGearBegin; i >= 1; i--)
    {
        // 先尝试 5001
        OptMemClkGear = i;
        MFGPOEOOutStream << "TuneMemClk: try MemClkGear: " << std::dec << OptMemClkGear << std::endl;
        std::cout << "TuneMemClk: try MemClkGear: " << std::dec << OptMemClkGear << std::endl;
        NVMLConfig.SetMemClkUpperLimit(vecMemClk[OptMemClkGear]); // 设置要尝试的 显存频率
        printMemClkUpperLimit(OptMemClkGear);
        sleep(3); // 延时使GPU工作稳定
        CHECK_TERMINATION_RETURN vecMemClk.size() - 1;

        global_measure_count++;
        IndexCtrl = global_measure_count % VECTOR_MEASURE_DATA_SIZE; // 当前控制区间
        // 测量一个窗口 (测量时间可以稍微长一点)
        printTimeStamp();
        MFGPOEOOutStream << "measureData..." << std::endl;
        std::cout << "measureData..." << std::endl;
        measureData(IndexCtrl, MeasureWindowDuration, true, vecSMClk[OptSMClkGear], vecMemClk[OptMemClkGear]);
        CHECK_TERMINATION_RETURN vecMemClk.size() - 1;

        // 计算性能指标
        PerfIndex = AnalysisKernelData(CoveredExeTimePct, MatchedExeTimePct, MatchedGapTimePct,ExeIndex, GapIndex, MatchedStreams, UnMatchedStreamExeTimePct, true, BaseData, vecMeasureData[IndexCtrl], GlobalConfig.ExeTimeRadius, GlobalConfig.GapTimeRadius, GlobalConfig.OptObj, GlobalConfig.objective);
        deltaExeTimePct = std::abs(BaseData.avgExeTimePct - vecMeasureData[IndexCtrl].avgExeTimePct);
        deltaExeTimePctPrev = deltaExeTimePct;
        MFGPOEOOutStream << "TuneMemClk: deltaExeTimePct: " << std::fixed << std::setprecision(2) << deltaExeTimePct * 100 << " %" << std::endl;

        if (PerfIndex < 0.0)
        {
            global_measure_count++;
            IndexCtrl = global_measure_count % VECTOR_MEASURE_DATA_SIZE; // 当前控制区间
            // 测量一个窗口 (测量时间可以稍微长一点)
            printTimeStamp();
            MFGPOEOOutStream << "measureData..." << std::endl;
            std::cout << "measureData..." << std::endl;
            measureData(IndexCtrl, MeasureWindowDuration, true, vecSMClk[OptSMClkGear], vecMemClk[OptMemClkGear]);
            CHECK_TERMINATION_RETURN vecMemClk.size() - 1;

            double tmpPerfIndex = AnalysisKernelData(CoveredExeTimePct, MatchedExeTimePct, MatchedGapTimePct,ExeIndex, GapIndex, MatchedStreams, UnMatchedStreamExeTimePct, true, BaseData, vecMeasureData[IndexCtrl], GlobalConfig.ExeTimeRadius, GlobalConfig.GapTimeRadius, GlobalConfig.OptObj, GlobalConfig.objective);
            deltaExeTimePct = std::abs(BaseData.avgExeTimePct - vecMeasureData[IndexCtrl].avgExeTimePct);
            deltaExeTimePctPrev = deltaExeTimePct;
            MFGPOEOOutStream << "TuneMemClk: deltaExeTimePct: " << std::fixed << std::setprecision(2) << deltaExeTimePct * 100 << " %" << std::endl;
            PerfIndex = std::max(PerfIndex, tmpPerfIndex);
            // PerfIndex = (PerfIndex + tmpPerfIndex) / 2;
        }

        // 如果性能损失较大, 就回退 OptMemClkGear
        if (PerfIndex > PerfLossConstraint || vecMeasureData[IndexCtrl].Obj > OptObj) {
            if (i == tryMemClkGearBegin) {
                OptMemClkGear = vecMemClk.size() - 1;
                NVMLConfig.ResetMemClkLimit();
                printMemClkUpperLimit(OptMemClkGear);
            } else {
                OptMemClkGear += 1;
                NVMLConfig.SetMemClkUpperLimit(vecMemClk[OptMemClkGear]);
                printMemClkUpperLimit(OptMemClkGear);
            }
            MFGPOEOOutStream << "TuneMemClk: 性能损失超过约束, 不再尝试更低显存频率" << std::dec << std::endl;
        }

        // 如果性能损失潜力已经利用了较多, 就不再尝试更低显存频率
        if (PerfIndex > PerfLossConstraint)
        {
            MFGPOEOOutStream << "TuneMemClk: 性能损失额度已利用较多, 不再尝试更低显存频率" << std::dec << std::endl;
            break;
        }
        
        OptObj = vecMeasureData[IndexCtrl].Obj;
        OptPerfIndex = PerfIndex;
        // 再尝试 810
        // 再尝试 405
    }

    MFGPOEOOutStream << "PID: OptMemClkGear = " << std::dec << OptMemClkGear << std::endl;
    std::cout << "PID: OptMemClkGear = " << std::dec << OptMemClkGear << std::endl;
    MFGPOEOOutStream << "TuneMemClk: done" << std::endl;
    return OptMemClkGear;
}

// wfr 20230208 预测模式, 仅预测某配置下的性能, 发送时间戳消息 并 打印绝对时间戳
void *PredictPerformance(void *arg)
{
    CHECK_TERMINATION_RETURN NULL; // 如果停止标志为 true 直接跳出

    // 初始化: 
    // float MeasureWindowDuration = 4.0;
    float MeasureWindowDuration = GlobalConfig.MeasureWindowDuration; // 测量窗口长度 (s)
    float prevMeasureWindowDuration = MeasureWindowDuration; // 上次 测量窗口长度 (s)
    float Pct_Inc = GlobalConfig.Pct_Inc; // < Pct_Inc 时 尝试增大窗口长度
    float Pct_Ctrl = GlobalConfig.Pct_Ctrl; // > Pct_Ctrl 时 进行 PID 控制, 更新最优能耗配置
    int BaseUpdateCycle = 2; // 基准更新频次
    // int BaseUpdateCycle = GlobalConfig.BaseUpdateCycle; // 基准更新频次
    int NumOptConfig = GlobalConfig.NumOptConfig; // 确定稳定所需的最近最优配置数量
    int NumCtrlPctErr = NumOptConfig;
    unsigned int NumKernels = 0;

    double PerfIndex = GlobalConfig.objective;
    double Objective = GlobalConfig.objective;
    float CoveredExeTimePct = 0.0; // 数据库覆盖到的 kernel数据 的百分比
    float MatchedExeTimePct = 0.0; // 在数据库中匹配到的 kernel数据 的百分比
    float MatchedGapTimePct = 0.0;  // 在数据库中匹配到的 Gap数据 的百分比
    float ExeIndex = 0.0, GapIndex = 0.0;
    unsigned int MatchedStreams = 0; // 在数据库中匹配到的 kernel数据 所在的 stream 的数量
    float UnMatchedStreamExeTimePct = 0.0; // CtrlData 中没有匹配到的 stream 的 ExeTime 占 CtrlData 总 ExeTime 的比例
    sumMeasurementTime = 0.0;

    int IndexCtrl, IndexBase;
    struct timeval StartTimeVal, EndTimeVal, TimeVal;

    // 修改频率配置 (频率配置从环境变量读取)
    if (TestedSMClk < GlobalConfig.MaxSMClk)
    {
        NVMLConfig.SetSMClkUpperLimit(TestedSMClk);
    }
    MFGPOEOOutStream << "PredictPerformance: TestedSMClk: " << std::dec << TestedSMClk << std::endl;
    std::cout << "PredictPerformance: TestedSMClk: " << std::dec << TestedSMClk << std::endl;

#if defined(SUPPORT_SET_MEMORY_CLOCK)
    if (TestedMemClk < GlobalConfig.MaxMemClk)
    {
        NVMLConfig.SetMemClkUpperLimit(TestedMemClk);
    }
    MFGPOEOOutStream << "PredictPerformance: TestedMemClk: " << std::dec << TestedMemClk << std::endl;
    std::cout << "PredictPerformance: TestedMemClk: " << std::dec << TestedMemClk << std::endl;
#endif

    for (size_t i = 0; i < 30; i++)
    {
        monitorIdle(-1, -1, false);
    }
    // sleep(6);
    // sleep(14);
    // sleep(18);
    // sleep(24); // 延时一段时间 避开一开始的不稳定阶段
    srand(time(0));

    MeasureWindowDuration = DetermineMeasurementDuration();

    BaseData.clear();
    CUPTIBufferCount = -1; // 全局变量
    global_measure_count = -1;
    global_measure_count++;
    IndexBase = global_measure_count % VECTOR_MEASURE_DATA_SIZE;

    printTimeStamp();
    MFGPOEOOutStream << "\nMeasureWindowDuration = " << std::fixed << std::setprecision(1) << 4 << std::endl;
    MFGPOEOOutStream << "measureBaseData..." << std::endl;
    std::cout << "measureBaseData..." << std::endl;
    measureBaseData(IndexBase, 4, 0.0); // wfr 20230422 首次测量的数据会很异常, 因此先 warm-up 一次
    CHECK_TERMINATION_RETURN NULL; // 如果停止标志为 true 直接跳出
    sleep(2);

    printTimeStamp();
    MFGPOEOOutStream << "\nMeasureWindowDuration = " << std::fixed << std::setprecision(1) << MeasureWindowDuration << std::endl;
    MFGPOEOOutStream << "measureBaseData..." << std::endl;
    std::cout << "measureBaseData..." << std::endl;
    measureBaseData(IndexBase, MeasureWindowDuration, 0.0);
    CHECK_TERMINATION_RETURN NULL; // 如果停止标志为 true 直接跳出
    BaseData = vecMeasureData[IndexBase]; // 初始化 Base 数据
    NumKernels += vecMeasureData[IndexBase].NumKernels;
    sleep(2);

    CoveredExeTimePct = 1.0;
    MatchedExeTimePct = 1.0;
    MatchedGapTimePct = 1.0;
    PerfIndex = 0.0;
    float prevMatchedExeTimePct = 0.0;
    float deltaExeTimePct = 0.0;
    float sumDeltaExeTimePct = 0.0;
    float tmpDiscount = 1.0;
    float tmpSleepTime = 0.0;
    int ViolateCount = 0;
    if (MeasureWindowDuration < 5)
    {
        ViolateCount = 1;
    } else
    {
        ViolateCount = 0;
    }
    // 先收集默认配置下base数据
    do {
        usleep(tmpSleepTime*1e6);

        printTimeStamp();
        MFGPOEOOutStream << "\nMeasureWindowDuration = " << std::fixed << std::setprecision(1) << MeasureWindowDuration << std::endl;
        MFGPOEOOutStream << "measureBaseData..." << std::endl;
        std::cout << "measureBaseData..." << std::endl;
        global_measure_count++;
        IndexBase = global_measure_count % VECTOR_MEASURE_DATA_SIZE;
        // measureBaseData(IndexBase, MeasureWindowDuration, BaseData.KernelsPerSec);
        measureBaseData(IndexBase, MeasureWindowDuration, 0.0);
        NumKernels += vecMeasureData[IndexBase].NumKernels;
        CHECK_TERMINATION_BREAK;

        MFGPOEOOutStream << "deltaExeTimePct: " << std::fixed << std::setprecision(2) << deltaExeTimePct * 100 << " %" << std::endl;

        PerfIndex = AnalysisKernelData(CoveredExeTimePct, MatchedExeTimePct, MatchedGapTimePct, ExeIndex, GapIndex, MatchedStreams, UnMatchedStreamExeTimePct, false, BaseData, vecMeasureData[IndexBase], GlobalConfig.ExeTimeRadius, GlobalConfig.GapTimeRadius, GlobalConfig.OptObj, GlobalConfig.objective);
        CHECK_TERMINATION_BREAK;

        prevMeasureWindowDuration = MeasureWindowDuration;
        if (CoveredExeTimePct < Pct_Inc || MatchedExeTimePct < Pct_Ctrl // 当覆盖度 或 匹配度都不足时, 增大测量区间时长
            || std::abs(PerfIndex) > 0.04 // 当性能指标不为 0 时增大测量区间时长
            || std::abs(ExeIndex) > 0.08 || std::abs(GapIndex) > 0.25
            || (BaseData.avgGPUUtil > 70 && BaseData.avgMemUtil > 50 && std::abs(GapIndex) > 0.05)
            || std::abs(BaseData.avgGPUUtil-vecMeasureData[IndexBase].avgGPUUtil)/(BaseData.avgGPUUtil+vecMeasureData[IndexBase].avgGPUUtil) > 0.06
        ) {
            ViolateCount++;

            if (ViolateCount >= 2)
            {
                // MeasureWindowDuration *= 1.618;
                MeasureWindowDuration *= 2;
                MeasureWindowDuration = std::min((float)GlobalConfig.WindowDurationMax, (float)MeasureWindowDuration);
                MFGPOEOOutStream << "MeasureWindowDuration Increase (PerfIndex): " << std::fixed << std::setprecision(1) << prevMeasureWindowDuration << " s --> " << MeasureWindowDuration << " s" << std::endl;
                ViolateCount = 0;
            }

            if (std::abs(ExeIndex) < 0.02 && std::abs(GapIndex) < 0.02)
            {
                tmpDiscount = 0.9;
            } else if (std::abs(ExeIndex) < 0.08 && std::abs(GapIndex) < 0.08)
            {
                tmpDiscount = 0.7;
            } else if (std::abs(ExeIndex) < 0.12 && std::abs(GapIndex) < 0.12)
            {
                tmpDiscount = 0.6;
            } else
            {
                tmpDiscount = 0.4;
            }

        } else {
            tmpDiscount = 0.9; // 这里 discount = 1.0 表示要强制融合 不衰减
        }
        
        BaseData.Merge(vecMeasureData[IndexBase], tmpDiscount, false, GlobalConfig.ExeTimeRadius, GlobalConfig.GapTimeRadius); // 融合 Base 历史数据

        MFGPOEOOutStream << "MatchedStreamsPct: " << std::fixed << std::setprecision(2) << (float)MatchedStreams / BaseData.map2Kernel.size() * 100 << " %" << std::endl;
    } while (!GlobalConfig.terminateThread 
        && global_measure_count < 5
        && (
        (CoveredExeTimePct < Pct_Inc || MatchedExeTimePct < Pct_Ctrl)
        || std::abs(PerfIndex) > 0.04 // 0.015 // 默认配置下性能损失指标接近 0%, 才停止数据库初始化
        || std::abs(ExeIndex) > 0.08 || std::abs(GapIndex) > 0.15
        // || global_measure_count < 2
        // || global_measure_count < 1
        )
    );
    std::cout << "PredictPerformance: 数据库构建完成" << std::endl;

    // 修改频率配置 (频率配置从环境变量读取)
    if (TestedSMClk < GlobalConfig.MaxSMClk)
    {
        NVMLConfig.SetSMClkUpperLimit(TestedSMClk);
    }
    MFGPOEOOutStream << "PredictPerformance: TestedSMClk: " << std::dec << TestedSMClk << std::endl;
    std::cout << "PredictPerformance: TestedSMClk: " << std::dec << TestedSMClk << std::endl;

#if defined(SUPPORT_SET_MEMORY_CLOCK)
    if (TestedMemClk < GlobalConfig.MaxMemClk)
    {
        NVMLConfig.SetMemClkUpperLimit(TestedMemClk);
    }
    MFGPOEOOutStream << "PredictPerformance: TestedMemClk: " << std::dec << TestedMemClk << std::endl;
    std::cout << "PredictPerformance: TestedMemClk: " << std::dec << TestedMemClk << std::endl;
#endif

    sleep(2); // 延时使GPU工作稳定
    // sleep(4); // 延时使GPU工作稳定
    OptSMClkGear = vecSMClk.size() - 1;
    for (int i = vecSMClk.size() - 1; i >= 0; --i)
    {
        if (TestedSMClk == vecSMClk[i]) {
            OptSMClkGear = i;
            break;
        }
    }

    OptMemClkGear = vecMemClk.size() - 1;
    for (int i = vecMemClk.size() - 1; i >= 0; --i)
    {
        if (TestedMemClk == vecMemClk[i]) {
            OptMemClkGear = i;
            break;
        }
    }

    // 预测性能指标
    int predict_count = 0;
    std::cout << "PredictPerformance: before while" << std::endl;
    while (!GlobalConfig.terminateThread) {
        // 通过 衰减系数 来控制数据库更新
        // if (predict_count != 0 && predict_count % BaseUpdateCycle == 0) {
        // if (predict_count != 0) {
        if ((predict_count != 0 && predict_count%BaseUpdateCycle == 0)
            || CoveredExeTimePct < Pct_Inc 
            || MatchedExeTimePct < Pct_Ctrl 
            || UnMatchedStreamExeTimePct > 0.1
        ) {
            MFGPOEOOutStream << "\nmeasureBaseData..." << std::endl;
            std::cout << "\nmeasureBaseData..." << std::endl;
            // if (BaseUpdateCycle == 1) {
            //     BaseUpdateCycle = std::min(GlobalConfig.BaseUpdateCycle, BaseUpdateCycle+3);
            // } else {
            //     BaseUpdateCycle = std::min(GlobalConfig.BaseUpdateCycle, BaseUpdateCycle+2);
            // }
            if (BaseUpdateCycle != GlobalConfig.BaseUpdateCycle)
            {
                BaseUpdateCycle = GlobalConfig.BaseUpdateCycle;
            }

            global_measure_count++;
            IndexBase = global_measure_count % VECTOR_MEASURE_DATA_SIZE;
            measureBaseData(IndexBase, MeasureWindowDuration, 0.0);
            NumKernels += vecMeasureData[IndexBase].NumKernels;
            CHECK_TERMINATION_BREAK;

            float tmpPerfIndex = AnalysisKernelData(CoveredExeTimePct, MatchedExeTimePct, MatchedGapTimePct, ExeIndex, GapIndex, MatchedStreams, UnMatchedStreamExeTimePct, false, BaseData, vecMeasureData[IndexBase], GlobalConfig.ExeTimeRadius, GlobalConfig.GapTimeRadius, GlobalConfig.OptObj, GlobalConfig.objective);
            CHECK_TERMINATION_BREAK;

            bool isRelative = true;
            float tmpDiscount = 1.0;
            if (std::abs(tmpPerfIndex) < 0.02)
            {
                tmpDiscount = 0.8;
                isRelative = false;
            } else if (std::abs(tmpPerfIndex) < 0.05)
            {
                tmpDiscount = 0.4;
                isRelative = true;
            } else if (std::abs(tmpPerfIndex) < 0.08)
            {
                tmpDiscount = 0.3;
                isRelative = true;
            } else if (std::abs(tmpPerfIndex) < 0.12)
            {
                tmpDiscount = 0.2;
                isRelative = true;
            } else
            {
                tmpDiscount = 0.2;
                isRelative = true;
            }

            BaseData.Merge(vecMeasureData[IndexBase], tmpDiscount, isRelative, GlobalConfig.ExeTimeRadius, GlobalConfig.GapTimeRadius); // 融合 Base 历史数据

            if (OptSMClkGear >= 0 && vecSMClk.size() > 0) {
                if (TestedSMClk < GlobalConfig.MaxSMClk)
                {
                    NVMLConfig.SetSMClkUpperLimit(TestedSMClk);
                }
                MFGPOEOOutStream << "PredictPerformance: TestedSMClk: " << std::dec << TestedSMClk << std::endl;
                std::cout << "PredictPerformance: TestedSMClk: " << std::dec << TestedSMClk << std::endl;
            }
#if defined(SUPPORT_SET_MEMORY_CLOCK)
            if (OptMemClkGear >= 0 && vecMemClk.size() > 0) {
                if (TestedMemClk < GlobalConfig.MaxMemClk)
                {
                    NVMLConfig.SetMemClkUpperLimit(TestedMemClk);
                }
                MFGPOEOOutStream << "PredictPerformance: TestedMemClk: " << std::dec << TestedMemClk << std::endl;
                std::cout << "PredictPerformance: TestedMemClk: " << std::dec << TestedMemClk << std::endl;
            }
#endif
            sleep(2);
            CHECK_TERMINATION_BREAK;
        }

        // 在给定配置下, 测量一个测量区间内的数据
        global_measure_count++;
        IndexCtrl = global_measure_count % VECTOR_MEASURE_DATA_SIZE; // 当前控制区间
        // 测量一个窗口 (测量时间可以稍微长一点)
        printTimeStamp();
        MFGPOEOOutStream << "measureData..." << std::endl;
        std::cout << "measureData..." << std::endl;
        float tmpScale = 1.0;
        // tmpScale = std::max((float)1.0, tmpScale);
        // tmpScale = std::min((float)(1.0+GlobalConfig.objective), tmpScale);
        // TestedSMClk TestedMemClk
        // GlobalConfig.MaxSMClk GlobalConfig.MaxMemClk
        if (TestedMemClk <= 810)
        {
            // tmpScale += 2;
            // tmpScale += 1.5;
            tmpScale += 1;
        }
        measureData(IndexCtrl, MeasureWindowDuration * tmpScale, true, vecSMClk[OptSMClkGear], vecMemClk[OptMemClkGear]);
        NumKernels += vecMeasureData[IndexCtrl].NumKernels;
        CHECK_TERMINATION_BREAK;

        // 计算性能指标
        PerfIndex = AnalysisKernelData(CoveredExeTimePct, MatchedExeTimePct, MatchedGapTimePct, ExeIndex, GapIndex, MatchedStreams, UnMatchedStreamExeTimePct, true, BaseData, vecMeasureData[IndexCtrl], GlobalConfig.ExeTimeRadius, GlobalConfig.GapTimeRadius, GlobalConfig.OptObj, GlobalConfig.objective);
        CHECK_TERMINATION_BREAK;
        predict_count++;

        deltaExeTimePct = std::abs(BaseData.avgExeTimePct - vecMeasureData[IndexBase].avgExeTimePct);
        MFGPOEOOutStream << "deltaExeTimePct: " << std::fixed << std::setprecision(2) << deltaExeTimePct * 100 << " %" << std::endl;

        // 如果匹配度满足阈值
        if (MatchedExeTimePct >= Pct_Ctrl || predict_count >=3) 
        {
            // 打时间戳: 预测完成
            // gettimeofday(&TimeVal, NULL);
            printTimeStamp();

            std::string Description = "PerformanceLoss = " + std::to_string(PerfIndex);
            TimeStamp(Description);
            // double tmpTime = (double)TimeVal.tv_sec + (double)TimeVal.tv_usec * 1e-6;
            // std::cout << "PredictPerformance: 性能损失预测完成时刻: " << std::fixed << std::setprecision(2) << tmpTime << " s; 性能损失: " << std::scientific << std::setprecision(4)  << PerfIndex << std::flush;
            MFGPOEOOutStream << "PredictPerformance: 性能损失 = " << std::fixed << std::setprecision(4)  << PerfIndex * 100 << " %" << std::endl << std::flush;

            break;
        }
    }

    // wfr 20230316 在线性能评估完成后重置频率, 以更快完成之后的负载, 尽快结束实验
    NVMLConfig.ResetPowerLimit();
    NVMLConfig.ResetSMClkLimit();
    NVMLConfig.ResetMemClkLimit();

    std::cout << "PredictPerformance 结束" << std::endl;
    return NULL;
}

// wfr 20220715 数据处理线程主函数
static void* threadAnalysis(void* pArg) {

    GlobalConfig.RefCountAnalysis++;

    pthread_t tid = pthread_self();
    std::cout << "threadAnalysis: 进入 tid = " << std::hex << tid << std::endl;

    // 获得 CUPTI 结果 index 等参数
    int index = ((THREAD_ANALYSIS_ARGS*)pArg)->index;
    double TimeStamp = ((THREAD_ANALYSIS_ARGS*)pArg)->TimeStamp;
    float SampleInterval = ((THREAD_ANALYSIS_ARGS*)pArg)->SampleInterval;
    float MeasureDuration = ((THREAD_ANALYSIS_ARGS*)pArg)->MeasureDuration;
    unsigned int SMClkUpperLimit = ((THREAD_ANALYSIS_ARGS*)pArg)->SMClkUpperLimit;
    unsigned int MemClkUpperLimit = ((THREAD_ANALYSIS_ARGS*)pArg)->MemClkUpperLimit;
    bool cupti_flag = ((THREAD_ANALYSIS_ARGS*)pArg)->cupti_flag;

    std::cout << "threadAnalysis: index = " << std::dec << index << std::endl;

    // 解析 CUPTI 结果
    std::cout << "threadAnalysis: Before Analysis" << std::endl;
    vecMeasureData[index].Analysis(TimeStamp, SampleInterval, MeasureDuration, SMClkUpperLimit, MemClkUpperLimit, cupti_flag);
    std::cout << "threadAnalysis: After Analysis" << std::endl;

    // 将 CUPTI 结果 写入文件
    // 这里需要加锁
    pthread_mutex_lock(&mutexOutStream);
    std::cout << "threadAnalysis: 获得文件锁" << std::endl;
    
    // 写文件
    MEASURE_DATA& Data = vecMeasureData[index];
    MFGPOEOOutStream << "Relative Time Stamp: " << std::fixed << std::setprecision(2) << Data.TimeStamp << " s" << std::endl;

    MFGPOEOOutStream << "Number of Streams: " << Data.mapStream.size() << std::endl;
    MFGPOEOOutStream << "Kinds of Kernels: " << Data.KernelKinds << std::endl;
    MFGPOEOOutStream << "Number of Kernels: " << Data.NumKernels << std::endl;
    MFGPOEOOutStream << "Accumulated Execution Time: " << std::scientific << std::setprecision(4) << Data.AccuExeTime << std::endl;
    MFGPOEOOutStream << "Kinds of Gaps: " << Data.GapKinds << std::endl;
    MFGPOEOOutStream << "Number of Gaps: " << Data.NumGaps << std::endl;
    MFGPOEOOutStream << "Accumulated Gap Time: " << std::scientific << std::setprecision(4) << Data.AccuGapTime << std::endl;

    for (auto& pairMapKernel : Data.map2Kernel) {
        int StreamID = pairMapKernel.first;
        for (auto& pairKernel : pairMapKernel.second) {
            std::string KernelID = pairKernel.first;

            MFGPOEOOutStream << "Stream " << std::dec << StreamID 
            << " Kernel " << KernelID << ": " << pairKernel.second.count << std::endl 
            
            << std::fixed << std::setprecision(2) << pairKernel.second.ExeTimeStableIndex * 100 << " %; "
            << std::scientific << std::setprecision(4) << pairKernel.second.minExeTime << " s; " 
            << pairKernel.second.avgExeTime << " s; " << pairKernel.second.maxExeTime << " s; "
            << pairKernel.second.sumExeTime << " s" << std::endl;
        }
    }

    for (auto& pairMapGap : Data.map2Gap) {
        int StreamID = pairMapGap.first;
        for (auto& pairGap : pairMapGap.second) {
            std::string GapName = pairGap.first;

            MFGPOEOOutStream << "Stream " << std::dec << StreamID 
            << " Gap " << GapName << ": " << pairGap.second.count << std::endl 
            
            << std::fixed << std::setprecision(2) << pairGap.second.GapTimeStableIndex * 100 << " %; "
            << std::scientific << std::setprecision(4) << pairGap.second.minGapTime << " s; " 
            << pairGap.second.avgGapTime << " s; " << pairGap.second.maxGapTime << " s; "
            << pairGap.second.sumGapTime << " s" << std::endl;
        }
    }

    MFGPOEOOutStream << "Number of Streams: " << Data.mapStream.size() << std::endl;
    for (auto iter = Data.mapStream.cbegin(); iter != Data.mapStream.cend(); ++iter) {
        // Stream ID0: count; minExeTime s; avgExeTime s; maxExeTime s
        MFGPOEOOutStream << "Stream " << iter->first << ": " << iter->second.KernelCount << "; " << std::scientific << std::setprecision(4) << iter->second.minExeTime << " s; " << iter->second.avgExeTime << " s; " << iter->second.maxExeTime << " s" << std::endl;
    }

    MFGPOEOOutStream << std::endl;

    pthread_mutex_unlock(&mutexOutStream);
    std::cout << "threadAnalysis: 释放文件锁" << std::endl;

    GlobalConfig.RefCountAnalysis--;

    std::cout << "threadAnalysis: 结束 tid = " << std::hex << tid << std::endl;

    pthread_exit(NULL);
}

void *MeasureKernelTrace(void *arg) {
    // cudaError_t error_id = cudaDriverGetVersion(&GlobalConfig.CUDADriverVersion);
    // std::cout << "error_id = " << std::dec << error_id << std::endl;
    // std::cout << "CUDADriverVersion = " << std::dec << GlobalConfig.CUDADriverVersion << std::endl;

    float measure_interval = 8.0; // 测量间隔 1 s

    // 文件读写需要锁 要保证 数据写入的 时间顺序

    uint64_t start, end; // 单位 ns
    start = 0;
    int64_t err_interval = 1 * 1000 * 1000; // 测量时长误差初值 1 ms
    double tmpTimeStamp;
    struct timeval tmpTimeVal;
    float MeasureDuration;

    pthread_t TIDAnalysis;
    pthread_attr_t AttrAnalysis;

    // 初始化条件变量 初值为 0
    pthread_mutex_init(&mutexOutStream, NULL);

    // 启动 CUPTI
    GlobalConfig.detachCupti = 0;
    GlobalConfig.istimeout = 0;
    GlobalConfig.isMeasuring = 1;
    GlobalConfig.tracingEnabled = 1;
    CUPTI_CALL(cuptiInitialize());
    CUPTI_CALL(cuptiGetTimestamp(&start));

    global_measure_count = -1; // 全局变量
    CUPTIBufferCount = -1; // 全局变量
    while (!GlobalConfig.terminateThread) {
        // Check the condition again
        CHECK_TERMINATION_BREAK;

        global_measure_count++;
        global_data_index = global_measure_count % VECTOR_MEASURE_DATA_SIZE; // 当前控制区间
        vecMeasureData[global_data_index].clear();

        // 延时一个控制周期
        std::cout << "global_measure_count: " << std::dec << global_measure_count << std::endl;
        std::cout << "global_data_index: " << std::dec << global_data_index << std::endl;
        std::cout << "延时..." << std::endl;
        std::cout << "measure_interval: " << std::fixed << std::setprecision(1) << measure_interval << " s" << std::endl;
        std::cout << "err_interval: " << std::scientific << std::setprecision(4) << err_interval * 1e-9 << " s" << std::endl;

        usleep((uint64_t)(measure_interval * 1e6) - (int64_t)(err_interval / 1000));
        std::cout << "延时结束" << std::endl;
        // Check the condition again
        CHECK_TERMINATION_BREAK;

        // 读取 CUPTI 测量数据
        CUPTI_CALL(cuptiActivityFlushAll(1));

        // 记录时间戳 和 测量时长
        CUPTI_CALL(cuptiGetTimestamp(&end));
        gettimeofday(&tmpTimeVal, NULL);
        tmpTimeStamp = (double)tmpTimeVal.tv_sec + (double)tmpTimeVal.tv_usec * 1e-6 - GlobalConfig.StartTimeStamp;
        std::cout << "start: " << std::dec << start << std::endl;
        std::cout << "end: " << std::dec << end << std::endl;
        MeasureDuration = (double)(end - start) * 1e-9;
        err_interval += (int64_t)(end - start) - (int64_t)(measure_interval * 1e9);
        start = end;

        std::cout << "tmpTimeStamp: " << std::fixed << std::setprecision(5) << tmpTimeStamp << " s" << std::endl;

        // Check the condition again
        CHECK_TERMINATION_BREAK;

        // 进行数据处理, 计算性能指标 加权运行时间
        // 这里放到单独线程处理数据
        pthread_attr_init(&AttrAnalysis);
        pthread_attr_setdetachstate(&AttrAnalysis, PTHREAD_CREATE_DETACHED);
        vecAnalysisArgs[global_data_index].index = global_data_index;
        vecAnalysisArgs[global_data_index].TimeStamp = tmpTimeStamp;
        vecAnalysisArgs[global_data_index].SampleInterval = SAMPLE_INTERVAL;
        vecAnalysisArgs[global_data_index].MeasureDuration = MeasureDuration;
        vecAnalysisArgs[global_data_index].SMClkUpperLimit = GlobalConfig.MaxSMClk;
        vecAnalysisArgs[global_data_index].MemClkUpperLimit = GlobalConfig.MaxMemClk;
        vecAnalysisArgs[global_data_index].cupti_flag = true;
        PTHREAD_CALL(pthread_create(&TIDAnalysis, &AttrAnalysis, threadAnalysis, &vecAnalysisArgs[global_data_index]));
    }

    // 停止 CUPTI
    GlobalConfig.detachCupti = 1;
    GlobalConfig.istimeout = 1;
    CUPTI_CALL(cuptiActivityFlushAll(1));
    std::cout << "callbackHandler: cuptiFinalize()" << std::endl;
    CUPTI_CALL(cuptiFinalize());
    PTHREAD_CALL(pthread_cond_broadcast(&GlobalConfig.mutexCondition));

    std::cout << "CUPTI 结束" << std::endl;

    GlobalConfig.isMeasuring = 0;
    GlobalConfig.tracingEnabled = 0;
    GlobalConfig.istimeout = 0;
    GlobalConfig.detachCupti = 0;
    GlobalConfig.subscriber = 0;

    std::cout << "MeasureKernelTrace 结束" << std::endl;

    return NULL;
}

// wfr 20220731 测试某种固定配置
void *StaticStrategy(void *arg) {

    // 设置GPU能耗配置
    // 调整能耗配置 (核心频率上限)
    int OptSMClkUpperLimit = 1605;
    NVMLConfig.SetSMClkUpperLimit(OptSMClkUpperLimit);
    printSMClkUpperLimit(OptSMClkUpperLimit);
    std::cout << "StaticStrategy: SMClkUpperLimit = " << std::dec << OptSMClkUpperLimit << " MHz" << std::endl;
    MFGPOEOOutStream << "StaticStrategy: SMClkUpperLimit = " << std::dec << OptSMClkUpperLimit << " MHz" << std::endl;

    return NULL;
}


// 优化代码: 尽量将功能放到子线程中, 避免增加应用执行时间
int STDCALL InitializeInjection(void)
// int __attribute__((__stdcall__)) InitializeInjection(void)
{
    printf("\nstart injection\n\n");

    if (GlobalConfig.initialized) {
        return 1;
    }
    // Init GlobalConfig
    globalInit();

    // Initialize Mutex
    PTHREAD_CALL(pthread_mutex_init(&GlobalConfig.mutexFinalize, 0));

    // 创建并打开输出文件
    MFGPOEOOutStream.open(GlobalConfig.OutPath, std::ifstream::out);
    MFGPOEOOutStream.close();
    // 再以 读写 方式 打开已经存在的文件
    MFGPOEOOutStream.open(GlobalConfig.OutPath, std::ifstream::out | std::ifstream::in | std::ifstream::binary);
    if(!MFGPOEOOutStream.is_open()){
        MFGPOEOOutStream.close();
        std::cout << "ERROR: failed to open output file" << std::endl;
        std::cout << "GlobalConfig.OutPath: " << GlobalConfig.OutPath << std::endl;
        return 1;
    }

    if (GlobalConfig.WorkMode == WORK_MODE::PID) {
        MFGPOEOOutStream << "-------- Model-Free GPU Online Energy Optimization System Output --------" << std::endl;
        MFGPOEOOutStream << std::endl;

        // 输出绝对起始时间戳
        struct timeval tmpTimeStamp;
        gettimeofday(&tmpTimeStamp, NULL);
        GlobalConfig.StartTimeStamp = (double)tmpTimeStamp.tv_sec + (double)tmpTimeStamp.tv_usec * 1e-6;
        MFGPOEOOutStream << "Start Time Stamp: " << std::fixed << std::setprecision(2) << GlobalConfig.StartTimeStamp << " s" << std::endl;
        MFGPOEOOutStream << std::endl;

        MFGPOEOOutStream << std::endl;
        MFGPOEOOutStream << "Relative Time Stamp: " << std::fixed << std::setprecision(2) << (double)0.0 << std::endl;
        MFGPOEOOutStream << std::endl;
    
    } else if (GlobalConfig.WorkMode == WORK_MODE::PREDICT) {
        MFGPOEOOutStream << "-------- Predict Performance Loss --------" << std::endl;
        MFGPOEOOutStream << std::endl;

        // 输出绝对起始时间戳
        struct timeval tmpTimeStamp;
        gettimeofday(&tmpTimeStamp, NULL);
        GlobalConfig.StartTimeStamp = (double)tmpTimeStamp.tv_sec + (double)tmpTimeStamp.tv_usec * 1e-6;
        MFGPOEOOutStream << "Start Time Stamp: " << std::fixed << std::setprecision(2) << GlobalConfig.StartTimeStamp << " s" << std::endl;
        MFGPOEOOutStream << std::endl;

        MFGPOEOOutStream << std::endl;
        MFGPOEOOutStream << "Relative Time Stamp: " << std::fixed << std::setprecision(2) << (double)0.0 << std::endl;
        MFGPOEOOutStream << std::endl;

    } else if (GlobalConfig.WorkMode == WORK_MODE::KERNEL) {
        MFGPOEOOutStream << "-------- Kernel Trace Data --------" << std::endl;
        MFGPOEOOutStream << std::endl;

        // 输出绝对起始时间戳
        struct timeval tmpTimeStamp;
        gettimeofday(&tmpTimeStamp, NULL);
        GlobalConfig.StartTimeStamp = (double)tmpTimeStamp.tv_sec + (double)tmpTimeStamp.tv_usec * 1e-6;
        MFGPOEOOutStream << "Start Time Stamp: " << std::fixed << std::setprecision(2) << GlobalConfig.StartTimeStamp << " s" << std::endl;
        MFGPOEOOutStream << std::endl;
    } else if (GlobalConfig.WorkMode == WORK_MODE::STATIC) {
        MFGPOEOOutStream << "-------- Static Strategy --------" << std::endl;
        MFGPOEOOutStream << std::endl;
    }

    registerAtExitHandler(); // 注册 注入结束时 要执行的函数

    // 这里启动 控制线程 / 仅测量线程
    // Launch the thread
    printf("before pthread_create\n");
    if (GlobalConfig.WorkMode == WORK_MODE::PID) {
        MFGPOEOOutStream << "WORK_MODE::PID" << std::endl;
        PTHREAD_CALL(pthread_create(&GlobalConfig.dynamicThread, NULL, AdaptiveWindowEnergyOptimization, NULL));
    } else if (GlobalConfig.WorkMode == WORK_MODE::PREDICT) {
        MFGPOEOOutStream << "WORK_MODE::PREDICT" << std::endl;
        PTHREAD_CALL(pthread_create(&GlobalConfig.dynamicThread, NULL, PredictPerformance, NULL));
    } else if (GlobalConfig.WorkMode == WORK_MODE::KERNEL) {
        MFGPOEOOutStream << "WORK_MODE::KERNEL" << std::endl;
        PTHREAD_CALL(pthread_create(&GlobalConfig.dynamicThread, NULL, MeasureKernelTrace, NULL));
    } else if (GlobalConfig.WorkMode == WORK_MODE::STATIC) {
        MFGPOEOOutStream << "WORK_MODE::STATIC" << std::endl;
        PTHREAD_CALL(pthread_create(&GlobalConfig.dynamicThread, NULL, StaticStrategy, NULL));
    }
    
    GlobalConfig.initialized = 1;

    return 1;
}