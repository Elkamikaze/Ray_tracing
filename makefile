all : ray_tracing_cpu.exe ray_tracing_mp.exe

ray_tracing_cpu.exe: ray_tracing_cpu.cpp
	g++  $< -o $@ -lglut -lGL

#ray_tracing_mpi.exe: ray_tracing_mpi.cpp
#	mpic++  $< -o $@ -lglut -lGL -O2

ray_tracing_mp.exe: ray_tracing_mp.cpp
	g++ -fopenmp $< -o $@ -lglut -lGL

clean:
	rm -rf *.exe
	
