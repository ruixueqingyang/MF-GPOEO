
/*******************************************************************************
Copyright(C), 2020-2020, 瑞雪轻飏
     FileName: PowerManager.cpp
       Author: 瑞雪轻飏
      Version: 0.01
Creation Date: 20200514
  Description: 能耗控制
       Others: 
*******************************************************************************/

#include "PowerManager.h"

const int RTX2080TiMaxPower = 280;
const int RTX2080TiDefaultPower = 250;
const int RTX2080TiMinSMClk = 300;
const int RTX2080TiMaxSMClk = 1830;
const int RTX2080TiSMClkStep = 15;
const int RTX2080TiBaseSMClk = 1350;

const int RTX3080TiMaxPower = 380;
const int RTX3080TiDefaultPower = 350;
const int RTX3080TiMinSMClk = 210;
const int RTX3080TiMaxSMClk = 2010; //2025
const int RTX3080TiSMClkStep = 15;
const int RTX3080TiBaseSMClk = 1365;

int POWER_MANAGER::initArg(int inDeviceIDNVML, EPOPT_NVML* inpNVML, int inTuneType, int inTuneArg){
    DeviceIDNVML = inDeviceIDNVML;
    TuneType = inTuneType;
    TuneArg = inTuneArg;
    return initArg(inpNVML);
}

int POWER_MANAGER::initArg(int inDeviceIDNVML, EPOPT_NVML* inpNVML, int inTuneType, float inLowerPct, float inUpperPct){
    DeviceIDNVML = inDeviceIDNVML;
    TuneType = inTuneType;
    LowerPct = inLowerPct;
    UpperPct = inUpperPct;
    return initArg(inpNVML);
}

int POWER_MANAGER::initArg(EPOPT_NVML* inpNVML){

    pMyNVML = inpNVML;
    pMyNVML->Init();

    unsigned int DeviceCount;
    nvmlResult = nvmlDeviceGetCount(&DeviceCount);
    if (NVML_SUCCESS != nvmlResult)
	{
		printf("Failed to get DeviceCount: %s\n", nvmlErrorString(nvmlResult));
		return -1;
	}
    std::cout << "POWER_MANAGER::initArg: nvmlDeviceGetCount: DeviceCount = " << DeviceCount << std::endl;

    nvmlResult = nvmlDeviceGetHandleByIndex(DeviceIDNVML, &device);
    if(NVML_SUCCESS != nvmlResult){
        std::cout << "Power Manager ERROR: Failed to get device handle (NVML ERROR INFO: " << nvmlErrorString(nvmlResult) << "). Invalid GPU index: -i " << optarg << "." << std::endl;
        exit(-1);
    }

    char DeviceName[512];
    nvmlResult = nvmlDeviceGetName(device, DeviceName, 512);
    if (NVML_SUCCESS != nvmlResult)
	{
		printf("Failed to get Device Name: %s\n", nvmlErrorString(nvmlResult));
		return -1;
	}
    // std::cout << "nvmlDeviceGetName: DeviceName = " << DeviceName << std::endl;
    GPUName = DeviceName;
    pMyNVML->Uninit();

    return 0;
}

int POWER_MANAGER::initCLI(EPOPT_NVML* inpNVML, int argc, char** argv){

    init(inpNVML);

    int err = 0;
    extern char *optarg;
    extern int optind, opterr, optopt;
    int c;
    const char *optstring = "i:t:p:l:u:";
    // -i DeviceIDNVML
    // -t 调节技术类型
    // -p 配置参数: F-F 对的 index 或 功率上限
    while ((c = getopt(argc, argv, optstring)) != -1) {
        switch (c) {
            case 'l':
                LowerPct = atof(optarg);
                if(0>LowerPct || LowerPct>100){
                    std::cout << "Power Manager ERROR: Invalid SM clock lower bound (-l " << optarg << ")." << std::endl;
                    err = -1;
                }
                break;
            case 'u': // 写到这里了
                UpperPct = atof(optarg);
                if(0>UpperPct || UpperPct>100){
                    std::cout << "Power Manager ERROR: Invalid SM clock upper bound (-u " << optarg << ")." << std::endl;
                    err = -1;
                }
                break;
            case 'i':
                DeviceIDNVML = atoi(optarg);
                if(DeviceIDNVML<0){
                    std::cout << "Power Manager ERROR: Invalid GPU index (-i " << optarg << ")." << std::endl;
                    err = -1;
                }
                break;
            case 't':
                if(strcmp("DVFS", optarg)==0){
                    TuneType = 0;
                }else if(strcmp("POWER", optarg)==0){
                    TuneType = 1;
                }else if(strcmp("SM_RANGE", optarg)==0){
                    TuneType = 2;
                }else{
                    std::cout << "Power Manager ERROR: Invalid tuning technology type (-t " << optarg << ")." << std::endl;
                    err = -1;
                }
                break;
            case 'p':
                TuneArg = atoi(optarg);
                if(TuneArg<0){
                    std::cout << "Power Manager ERROR: Invalid configuration parameter (-p " << optarg << ")." << std::endl;
                    err = -1;
                }
                break;
            case '?':
                printf("error optopt: %c\n", optopt);
                printf("error opterr: %d\n", opterr);
                err = -1;
                break;
        }
    }

    if(err!= 0) exit(err);
    err = initArg(inpNVML);
    if(err!= 0) exit(err);

    return 0;
}


int POWER_MANAGER::ResetSMClkRange(){

    pMyNVML->Init();
    nvmlResult = nvmlDeviceGetHandleByIndex(DeviceIDNVML, &device);
    if(NVML_SUCCESS != nvmlResult){
        std::cout << "Power Manager ERROR: Failed to get device handle (NVML ERROR INFO: " << nvmlErrorString(nvmlResult) << "). Invalid GPU index: -i " << optarg << "." << std::endl;
        exit(-1);
    }

    nvmlResult = nvmlDeviceResetGpuLockedClocks(device);
    if(NVML_SUCCESS != nvmlResult){
        printf("Power Manager ERROR: Failed to reset SM clock range (NVML ERROR INFO: %s)\n", nvmlErrorString(nvmlResult));
        exit(-1);
    }

    std::cout << "Power Manager: Reset SM clock" << std::endl;

    pMyNVML->Uninit();

    return 0;
}

int POWER_MANAGER::ResetMemClkRange(){

    pMyNVML->Init();
    nvmlResult = nvmlDeviceGetHandleByIndex(DeviceIDNVML, &device);
    if(NVML_SUCCESS != nvmlResult){
        std::cout << "Power Manager ERROR: Failed to get device handle (NVML ERROR INFO: " << nvmlErrorString(nvmlResult) << "). Invalid GPU index: -i " << optarg << "." << std::endl;
        exit(-1);
    }

    nvmlResult = nvmlDeviceResetMemoryLockedClocks(device);
    if(NVML_SUCCESS != nvmlResult){
        printf("Power Manager Warning: Failed to reset Mem clock range (NVML ERROR INFO: %s)\n", nvmlErrorString(nvmlResult));
        // exit(-1);
    }

    std::cout << "Power Manager: Reset Mem clock" << std::endl;

    pMyNVML->Uninit();

    return 0;
}

int POWER_MANAGER::Reset(){
    ResetSMClkRange();
    ResetMemClkRange();
    return 0;
}

std::string POWER_MANAGER::GetGPUName(){
    return GPUName;
}