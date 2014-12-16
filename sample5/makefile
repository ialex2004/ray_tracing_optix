ARCH = 30
OPTIX = /opt/optix

.SUFFIXES: .ptx

all: sample5

sample5: sample5.o constantbg.ptx normal_shader.ptx pinhole_camera.ptx sphere.ptx
	nvcc -arch=sm_$(ARCH) $< -o $@ -L$(OPTIX)/lib64 -loptix -Xlinker -rpath=$(OPTIX)/lib64 -L$(OPTIX)/SDK-precompiled-samples -lsutil -Xlinker -rpath=$(OPTIX)/SDK-precompiled-samples

constantbg.ptx: constantbg.cu
	nvcc -I$(OPTIX)/include -O3 -arch=sm_$(ARCH) --use_fast_math -ptx -c $< -o $@

normal_shader.ptx: normal_shader.cu
	nvcc -I$(OPTIX)/include -O3 -arch=sm_$(ARCH) --use_fast_math -ptx -c $< -o $@

pinhole_camera.ptx: pinhole_camera.cu
	nvcc -I$(OPTIX)/include -O3 -arch=sm_$(ARCH) --use_fast_math -ptx -c $< -o $@

sphere.ptx: sphere.cu
	nvcc -I$(OPTIX)/include -O3 -arch=sm_$(ARCH) --use_fast_math -ptx -c $< -o $@

sample5.o: sample5.c
	gcc -I$(OPTIX)/include -I$(OPTIX)/SDK/sutil -O3 -c $< -o $@

clean:
	rm -rf sample5 *.o *.ptx

test: sample5
	./sample5 --file out.ppm
