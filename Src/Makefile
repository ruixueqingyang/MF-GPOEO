CUDA_VERSION = 11.7.1

# OPT_CUDA_DIR = /opt/cuda-${CUDA_VERSION}
OPT_CUDA_DIR = /opt/cuda
ifeq (${OPT_CUDA_DIR}, ${wildcard ${OPT_CUDA_DIR}})
#目录存在
	CUDA_DIR = ${OPT_CUDA_DIR}
else
#目录不存在
endif

# LOCAL_CUDA_DIR = /usr/local/cuda-${CUDA_VERSION}
LOCAL_CUDA_DIR = /usr/local/cuda
ifeq (${LOCAL_CUDA_DIR}, ${wildcard ${LOCAL_CUDA_DIR}})
#目录存在
	CUDA_DIR = ${LOCAL_CUDA_DIR}
endif

CUDA_INC = $(CUDA_DIR)/include
CUDA_LIB = $(CUDA_DIR)/lib
CUDA_STUBS = $(CUDA_DIR)/lib64/stubs
CUPTI_LIB = $(CUDA_DIR)/extras/CUPTI/lib64
CUPTI_INC = $(CUDA_DIR)/extras/CUPTI/include

CC = g++
NVCC = $(CUDA_DIR)/bin/nvcc
DEBUG_FLAGS = -O3 # -g -O0
CFLAGS = ${DEBUG_FLAGS} -D SUPPORT_SET_MEMORY_CLOCK # 如果软硬件平台支持调整显存频率则定义宏
PIC_FLAG = -Xcompiler -fPIC
NVIDIA_LIBS = -L${CUDA_LIB} -L${CUPTI_LIB} -L${CUDA_STUBS} -lcudart -lpthread -lcupti -lnvidia-ml # -lcuda -lnvperf_target -lnvperf_host
RPATH = -Wl,-rpath=${CXX_LIB_DIR}

SRC_DIR = .
INCS = -I${SRC_DIR} -I$(CUDA_INC) -I${CUPTI_INC}
SRC = libGPOEO.cpp analysis.cpp measure.cpp kernel_data.cpp is_stable.cpp global_var.cpp Msg2EPRT.cpp
OBJ = $(SRC:.cpp=.o)
TARGET = libGPOEO.so

all: $(TARGET)

$(TARGET): $(OBJ)
	echo CUDA_DIR = ${CUDA_DIR}
	${NVCC} -o $@ $^ $(CFLAGS) -shared $(INCS) $(NVIDIA_LIBS)
	$(info export CUDA_INJECTION64_PATH=<full_path>/libGPOEO.so and run any CUDA sample with runtime more than 10 seconds(Graphics/Simulations samples) for demonstration)
	rm -f *.o

%.o: ${SRC_DIR}/%.cpp
	${NVCC} -o $@ -c $< $(CFLAGS) ${PIC_FLAG} ${INCS}

clean:
	rm -f *.so *.o *.bin

