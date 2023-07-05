CUDA_DIR = /opt/cuda
CUDA_INC = $(CUDA_DIR)/include
CUDA_GDK = $(CUDA_DIR)/gdk
CUDA_LIB = $(CUDA_DIR)/lib64
CUDA_STUBS = $(CUDA_DIR)/lib64/stubs

CC = g++
CFLAGS = -Wall -O3 -Wno-unused-parameter
SPECIALFLAGS = -lnvidia-ml -lpthread

SRC = main.cpp PowerManager.cpp
TARGET = PerfMeasure.bin

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -I$(CUDA_INC) -I$(CUDA_GDK) -L$(CUDA_LIB) -L$(CUDA_STUBS) -lcudart -lcuda $(SPECIALFLAGS) -o $@ $(SRC)

clean:
	rm -f *.o *.bin