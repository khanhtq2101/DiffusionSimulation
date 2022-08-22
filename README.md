Capstone Project: Diffusion Simulation with Cuda, MPI
------------------------------------------------------------------------------------------------------------------------
https://github.com/khanhtq2101/ParallelDiffusionSimulation.git  
Implementing Gauss Seidel algorithm by parallel programming with cuda and demo visualization.

How to run?  
------------------------------------------------------------------------------------------------------------------------
**1. CUDA:**
 - Create result folder:  
```
   mkdir -p ./results/CUDA
```
 - Compile and run the GaussSeidel.cu file:
``` 
   nvcc -o GaussSeidel_CUDA GaussSeidel.cu  
   ./GaussSeidel_CUDA
```
The output will be saved on /results/CUDA folder.  

**2. MPI:** 
 - Create result folder:
 ```
   mkdir -p ./results/MPI
 ```
  - Compile the GaussSeidel_MPI.c file:
``` 
   mpicc -o GaussSeidel_MPI GaussSeidel_MPI.c  
```
 - Run the compiled file:
 ```
   mpirun -np 16 GaussSeidel_MPI
 ```
 The output will be saved on /results/MPI folder.  
 
**3. Display the results:**  
- Run file text_to_imgage.py file to convert result in text to image.  
```
   python text_to_image.py 
```
- Run file display.py if you want to display process in some iterations.
```
   python display.py
```  
- Run file create_gif.py to create gif for diffusion process.  
```
   python create_gif.py 
```

**4. Results:**  
* Top down visulization: Initialization: 80 at centers and 25 around. Boundary condition: 100 at top right and 0 at bottom left.  
![](images/download.png)  
Gift display:  
![](images/top_down.gif)
* Top right visualization: Initialization: 80 at centers and 25 around.
Boundary condition: 100 at top down, and 0 at left and right side.  
![](images/top_right.gif)



