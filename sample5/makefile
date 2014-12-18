ARCH = 35
OPTIX = /opt/optix
NVCC = /opt/cuda_/bin/nvcc

.SUFFIXES: .ptx

all: sample5

sample5: sample5.o constantbg.ptx normal_shader.ptx pinhole_camera.ptx sphere.ptx
	$(NVCC) -arch=sm_$(ARCH) $< -o $@ -L$(OPTIX)/lib64 -loptix -Xlinker -rpath=$(OPTIX)/lib64 -L$(OPTIX)/SDK-precompiled-samples -lsutil -Xlinker -rpath=$(OPTIX)/SDK-precompiled-samples

constantbg.ptx: constantbg.cu
	$(NVCC) -I$(OPTIX)/include -O3 -arch=sm_$(ARCH) --use_fast_math -ptx -c $< -o $@

normal_shader.ptx: normal_shader.cu
	$(NVCC) -I$(OPTIX)/include -O3 -arch=sm_$(ARCH) --use_fast_math -ptx -c $< -o $@

pinhole_camera.ptx: pinhole_camera.cu
	$(NVCC) -I$(OPTIX)/include -O3 -arch=sm_$(ARCH) --use_fast_math -ptx -c $< -o $@

sphere.ptx: sphere.cu
	$(NVCC) -I$(OPTIX)/include -O3 -arch=sm_$(ARCH) --use_fast_math -ptx -c $< -o $@

sample5.o: sample5.cpp
	$(NVCC) -I$(OPTIX)/include -arch=sm_$(ARCH) -I$(OPTIX)/SDK/sutil -O3 -c $< -o $@

clean:
	rm -rf sample5 *.o *.ptx

test: clean sample5
	./sample5 --file out.ppm