/*******************************************************************************
Copyright(C), 2022-2022, 瑞雪轻飏
     FileName: CUPTIBuffer.h
       Author: 瑞雪轻飏
      Version: 0.01
Creation Date: 20221014
  Description: 1. 定义 CUPTI 测量时 返回给用户的 缓冲区
       Others: //其他内容说明
*******************************************************************************/

#ifndef __CUPTI_BUFFER_H
#define __CUPTI_BUFFER_H

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <iomanip>
#include <iostream>
#include <vector>
#include <system_error>
#include <string.h>
#include <vector>
#include <assert.h>
#include <math.h>
#include <fstream>
#include <sstream>
#include <math.h>
#include <map>
#include <unordered_map>
#include <iterator>
#include <algorithm>

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

#include <nvml.h>
#include <cupti.h>
#include <cuda.h>
#include <cuda_runtime.h>

#define CUPTI_CALL(call)                                                       \
do {                                                                           \
    CUptiResult _status = call;                                                \
    if (_status != CUPTI_SUCCESS) {                                            \
        const char *errstr;                                                    \
        cuptiGetResultString(_status, &errstr);                                \
        fprintf(stderr, "%s:%d: error: function %s failed with error %s.\n",   \
                __FILE__, __LINE__, #call, errstr);                            \
        exit(-1);                                                              \
    }                                                                          \
} while (0)

#define PTHREAD_CALL(call)                                                         \
do {                                                                               \
    int _status = call;                                                            \
    if (_status != 0) {                                                            \
        fprintf(stderr, "%s:%d: error: function %s failed with error code %d.\n",  \
                __FILE__, __LINE__, #call, _status);                               \
        exit(-1);                                                                  \
    }                                                                              \
} while (0)

// 一个测量缓冲的数据头
struct CUPTI_BUFFER_HEAD
{
    // bool isEmpty; // 是否为空
    int index; // 当前缓冲区的 index
    // uint64_t StartTimeStamp; // 开始测量时间戳
    // uint64_t EndTimeStamp; // 开始测量时间戳
};

// 数据 8 Byte 对齐
#define ALIGN_SIZE (8)
#define ALIGN_BUFFER(buffer, align)                                            \
  (((uintptr_t) (buffer) & ((align)-1)) ? ((buffer) + (align) - ((uintptr_t) (buffer) & ((align)-1))) : (buffer))

class CUPTI_BUFFER {
public:
    uint8_t* ptrOrigin;
    uint8_t* ptrAligned;
    bool isEmpty;
    CUcontext Ctx;
    uint32_t StreamID;
    size_t BufSize, ValidSize;
    uint64_t StartTimeStamp, EndTimeStamp;

    void reset_members() {
        isEmpty = false;
        Ctx = NULL;
        StreamID = 0;
        ValidSize = 0;
        StartTimeStamp = 0;
        EndTimeStamp = 0;
    }

    void free_buffer() {
        if (ptrOrigin != NULL) {
            free(ptrOrigin);
            ptrOrigin = NULL;
            ptrAligned = NULL; 
        }
        BufSize = 0;
        ValidSize = 0;
    }

    void clear() {
        reset_members();
        free_buffer();
    }

    CUPTI_BUFFER() {
        ptrOrigin = NULL;
        ptrAligned = NULL;
        BufSize = 0;
        ValidSize = 0;
        reset_members();
    }

    void init(uint32_t NumBytes, uint32_t index) {
        reset_members();
        int tmp = (int)(sizeof(CUPTI_BUFFER_HEAD) + ALIGN_SIZE - 1) / ALIGN_SIZE + 2; // 在开头多分配一些空
        ptrOrigin = (uint8_t*)calloc(NumBytes + tmp * ALIGN_SIZE, sizeof(uint8_t));
        ptrAligned = ALIGN_BUFFER(ptrOrigin, ALIGN_SIZE);
        tmp = ((int)(sizeof(CUPTI_BUFFER_HEAD) + ALIGN_SIZE - 1) / ALIGN_SIZE) * ALIGN_SIZE;
        ptrAligned = &(ptrAligned[tmp]); // 开头空出 1个 CUPTI_BUFFER_HEAD 的空间 用来保存缓冲数据头信息
        // 初始化缓冲数据头
        CUPTI_BUFFER_HEAD* pMeasureHead = (CUPTI_BUFFER_HEAD*)(ptrAligned) - 1;
        // pMeasureHead->isEmpty = true;
        pMeasureHead->index = index;
        BufSize = NumBytes;
        isEmpty = true;
    }

    CUPTI_BUFFER(uint32_t NumBytes, uint32_t index) {
        init(NumBytes, index);
    }
};

class CUPTI_BUFFER_MANAGER {
public:
    std::vector<CUPTI_BUFFER> vecBuf;
    uint32_t ValidIndex;
    uint32_t ValidSize;
    uint32_t BufSize;
    pthread_mutex_t mutexBuffer;

    void init(uint32_t NumBuffers, uint32_t NumBytes) {
        BufSize = NumBytes;
        mutexBuffer = PTHREAD_MUTEX_INITIALIZER;

        vecBuf.resize(NumBuffers);
        for (int i = 0; i < NumBuffers; ++i) {
            vecBuf[i].init(BufSize, i);
        }

        ValidSize = NumBuffers;
        ValidIndex = 0;
        // ValidIndex = 1;
    }

    void clear() {
        // 释放 CUPTI 数据缓冲空间
        for (auto Buf : vecBuf) {
            Buf.clear();
        }
        vecBuf.clear();
    }

    // 获得空闲的 buffer 的 index
    uint32_t getValidBufferIndex() {
        pthread_mutex_lock(&mutexBuffer);

        if (vecBuf[ValidIndex].isEmpty == false) {
            std::cout << "getValidIndex: 错误, 没找到空闲的 buffer !" << std::endl;
            exit(1);
        }
        vecBuf[ValidIndex].isEmpty = false; // 将 buffer 设置为在使用中
        uint32_t prevValidIndex = ValidIndex;
        --ValidSize;

        // 更新 ValidSize
        if (ValidSize == 0) { // 分配新空间
            ValidSize = vecBuf.size();
            vecBuf.resize(2 * vecBuf.size());
            // 对新增的 buffer 初始化
            for (int i = ValidSize; i < vecBuf.size(); ++i) {
                vecBuf[i].init(BufSize, i);
            }
        }

        // 这里找到新的 ValidIndex
        for (int i = 0; i < vecBuf.size() - 1; ++i) {
            ValidIndex = (++ValidIndex) % vecBuf.size();
            if (vecBuf[ValidIndex].isEmpty == true) {
                break;
            }
        }

        pthread_mutex_unlock(&mutexBuffer);

        return prevValidIndex;
    }

    // 释放 buffer
    void releaseBuffer(uint32_t index) {
        pthread_mutex_lock(&mutexBuffer);
        vecBuf[index].reset_members();
        vecBuf[index].isEmpty = true;
        ++ValidSize;
        pthread_mutex_unlock(&mutexBuffer);
    }
};


#endif
