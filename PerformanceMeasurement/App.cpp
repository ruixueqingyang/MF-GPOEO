/*******************************************************************************
Copyright(C), 2020-2020, 瑞雪轻飏
     FileName: App.cpp
       Author: 瑞雪轻飏
      Version: 0.01
Creation Date: 20200704
  Description: 用来测试能耗-性能测量程序的 DAEMON 功能
       Others: 
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <system_error>
#include <string.h>
#include <vector>
#include <assert.h>
#include <math.h>
#include <fstream>

#include <pthread.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <semaphore.h>


#include "Msg2EPRT.h"

int main(int argc, char** argv){

    std::string Msg;

    SetSMClkRange(20.0, 20.0);
    std::cout << "APP: SetSMClkRange(20.0, 20.0)" << std::endl;
    std::cout << "APP: sleep(1)" << std::endl;
    sleep(1);

    Msg = "./test1.txt";
    ResetMeasurement(Msg);

    StartMeasurement();
    std::cout << "APP: StartMeasurement()" << std::endl;
    Msg = "DataTransStart";
    TimeStamp(Msg);
    std::cout << "APP: DataTransStart" << std::endl;

    std::cout << "APP: sleep(2)" << std::endl;
    sleep(2);
    Msg = "DataTransEnd";
    TimeStamp(Msg);
    std::cout << "APP: DataTransEnd" << std::endl;

    SetSMClkRange(70.0, 70.0);
    std::cout << "APP: SetSMClkRange(70.0, 70.0)" << std::endl;
    Msg = "ComputeStart";
    TimeStamp(Msg);
    std::cout << "APP: ComputeStart" << std::endl;

    std::cout << "APP: sleep(2)" << std::endl;
    sleep(2);
    Msg = "ComputeEnd";
    TimeStamp(Msg);
    std::cout << "APP: ComputeEnd" << std::endl;

    StopMeasurement();
    std::cout << "APP: StopMeasurement()" << std::endl;




    std::cout << "APP: sleep(5)" << std::endl;
    sleep(5);




    Msg = "./test2.txt";
    std::cout << std::endl;
    std::cout << "APP: ResetMeasurement()" << std::endl;
    ResetMeasurement(Msg);
    std::cout << "APP: StartMeasurement()" << std::endl;
    StartMeasurement();
    SetSMClkRange(90.0, 90.0);
    sleep(5);
    StopMeasurement();
    std::cout << "APP: StopMeasurement()" << std::endl;

    ExitMeasurement();
    std::cout << "APP: ExitMeasurement()" << std::endl;

    return 0;
}