CUDA_DIR = /opt/cuda
CUDA_INC = $(CUDA_DIR)/include
CUDA_LIB = $(CUDA_DIR)/lib
CUDA_STUBS = $(CUDA_DIR)/lib64/stubs
CUPTI_LIB = $(CUDA_DIR)/extras/CUPTI/lib64
CUPTI_INC = $(CUDA_DIR)/extras/CUPTI/include

NVCC = $(CUDA_DIR)/bin/nvcc
DEBUG_FLAGS = -O3
# Define the macro "SUPPORT_SET_MEMORY_CLOCK" to enable memory frequency adjustment, if the NVIDIA GPU supports memory frequency adjustment.
CFLAGS = ${DEBUG_FLAGS} -D SUPPORT_SET_MEMORY_CLOCK
PIC_FLAG = -Xcompiler -fPIC
NVIDIA_LIBS = -L${CUDA_LIB} -L${CUPTI_LIB} -L${CUDA_STUBS} -lcudart -lpthread -lcupti -lnvidia-ml
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
	$(info export CUDA_INJECTION64_PATH=<full_path>/libGPOEO.so and run any CUDA application)
	rm -f *.o

%.o: ${SRC_DIR}/%.cpp
	${NVCC} -o $@ -c $< $(CFLAGS) ${PIC_FLAG} ${INCS}

clean:
	rm -f *.so *.o *.bin

