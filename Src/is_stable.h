/*******************************************************************************
Copyright(C), 2022-2022, 瑞雪轻飏
     FileName: is_stable.h
       Author: 瑞雪轻飏
      Version: 0.01
Creation Date: 20220615
  Description: 判断应用是否进入稳定阶段
       Others: 
*******************************************************************************/


#ifndef __IS_STABLE_H
#define __IS_STABLE_H

#include "measure.h"

// 计算两组 NVML 数据之间的百分比距离
float NVMLDistance(MEASURE_DATA& BaseData,MEASURE_DATA& Data);

// 固定长度循环队列, 即所有元素都会被初始化
template <class T>
class CIRCULAR_QUEUE {
public:
    unsigned int size, capacity, head;
    std::vector<T> vec;

    void push(T val) {
        if (size < capacity) {
            vec[size] = val;
            ++size;
        } else {
            vec[head] = val;
            head = (head+1) % capacity;
        }
    }

    // 重载 [] 允许正整数/0/负整数索引, [-1]表示循环倒数第一个元素
    // T operator[](int in_index) {
    //     int index;
    //     if (in_index >= 0)
    //     {
    //         index = (head + in_index) % size;
    //     } else {
    //         int count = std::ceil((float)(-1.0) * (float)in_index / (float)size);
    //         index = (head + in_index + count * size) % size;
    //     }
        
    //     return vec[index];
    // }

    T& operator[](int in_index) {
        int index;
        if (in_index >= 0)
        {
            index = (head + in_index) % size;
        } else {
            int count = std::ceil((float)(-1.0) * (float)in_index / (float)size);
            index = (head + in_index + count * size) % size;
        }
        
        return vec[index];
    }

    void init(unsigned int in_capacity, T val) {
        size = 0;
        head = 0;
        capacity = in_capacity;
        vec.clear();
        vec.resize(capacity, val);
    }

    CIRCULAR_QUEUE(unsigned int in_capacity, T val) {
        init(in_capacity, val);
    }
    // ~CIRCULAR_QUEUE();
};

// 判断稳定性
unsigned int judgeConfigStability(unsigned int judgeLen, int NumGearsThreshold, int CtrlCount, double sumMeasurementTime, CIRCULAR_QUEUE<unsigned int>& queOptSMClkGear, double CtrlPctErrThreshold, CIRCULAR_QUEUE<double>& queCtrlPctErr, CIRCULAR_QUEUE<double>& queOptObj);


#endif