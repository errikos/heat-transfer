CUDA_INSTALL_PATH = /opt/cuda
CXX = g++
OPTFLAG = -O2 -fomit-frame-pointer -ftree-vectorize -ftree-vectorizer-verbose=0  -funroll-loops
NVCC = ${CUDA_INSTALL_PATH}/bin/nvcc
INCDIR = ./include
FLAGS = ${OPTFLAG} -I${CUDA_INSTALL_PATH}/include -I${INCDIR} -Wall -g
NVFLAGS = -O2 -I${CUDA_INSTALL_PATH}/include -I${INCDIR} --compiler-options -fno-strict-aliasing --ptxas-options=-v -g
BITS = $(shell getconf LONG_BIT)
ifeq (${BITS},64)
	LIBSUFFIX := 64
endif
LFLAGS = -L${CUDA_INSTALL_PATH}/lib${LIBSUFFIX} -lm -lstdc++ -lcudart
CLEAN_FILES = cuda cuda.o update.o

cuda: cuda.o update.o
	${CXX} ${LFLAGS} -o $@ $^

cuda.o: cuda.cc
	${CXX} -c ${FLAGS} -o $@ $^

update.o: update.cu
	${NVCC} ${NVFLAGS} -DUNIX -c $^ -o $@

clean:
	\rm -f $(CLEAN_FILES)

rebuild: clean cuda
