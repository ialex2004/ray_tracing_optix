ARCH = 30
OPTIX = /opt/optix

NUM_SPHERES=10
NUM_LIGHTS=2
WIDTH   = 1920
HEIGHT  = 1080
FILE_NAME = ppm/1.ppm

.SUFFIXES: .ptx

all: ray_tracing

ray_tracing: ray_tracing.o ray_tracing.ptx ray_tracing.o
	nvcc -arch=sm_$(ARCH) $< -o $@ -L$(OPTIX)/lib64 -loptix -Xlinker -rpath=$(OPTIX)/lib64 -L$(OPTIX)/SDK-precompiled-samples -lsutil -Xlinker -rpath=$(OPTIX)/SDK-precompiled-samples

ray_tracing.ptx: ray_tracing.cu
	nvcc -I$(OPTIX)/include -O3 -arch=sm_$(ARCH) --use_fast_math -ptx -c $< -o $@

ray_tracing.o: ray_tracing.c
	gcc -I$(OPTIX)/include -I$(OPTIX)/SDK/sutil -O3 -c $< -o $@

clean:
	rm -rf ray_tracing ray_tracing.o ray_tracing.ptx

test: ray_tracing
	mkdir -p ppm && ./ray_tracing $(NUM_SPHERES) $(NUM_LIGHTS) $(WIDTH) $(HEIGHT) $(FILE_NAME)

