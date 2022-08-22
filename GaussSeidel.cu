#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <cuda.h>
#include <string.h>

#define M 256
#define N 256
#define Delta 0.001

#define BlocksX 16
#define ThreadsX 8
#define BlocksY 16
#define ThreadsY 8

#define U 100
#define D 100
#define L 0
#define R 0

void initialize(float *C);
void write2File(float *C, char name[]);
__global__ void redUpdate(float *C_gpu, float *e_gpu, int nX, int nY);
__global__ void blackUpdate(float *C_gpu, float *e_gpu, int nX, int nY);

/*
this function will find row-wise maximun in interval of size stride
maximum of this interval is stored in its first position
*/
__global__ void rowWiseReduction(float *e_gpu, int stride);
/*
this function will find column-wise maximun in interval of size stride
maximum of this interval is stored in its first position
*/
__global__ void columnWiseReduction(float *e_gpu, int stride);


int main(int argc, char *argv[]) {
	//Initialize C in cpu
	float *C;
	C = (float *)malloc((M * N) * sizeof(float));
	initialize(C);
	write2File(C, "00000.txt");

	//Send C to gpu
	float *C_gpu;
	cudaMalloc((void**) &C_gpu, (M*N)*sizeof(float));
	cudaMemcpy(C_gpu, C, (M*N)*sizeof(float), cudaMemcpyHostToDevice);
	
	//define size of grids and blocks
	dim3 dimBlock(ThreadsX, ThreadsY);
	dim3 dimGrid(BlocksX, BlocksY);
	
  /*
	stopping variable delta
  delta_gpu is 2D array, stores local maximum changing of threads
  delta_gpu(i, j) is local maximum of thread (i, j)
  */
	float *delta_gpu, delta = 1;
	cudaMalloc((void**) &delta_gpu, (BlocksX*BlocksY*ThreadsX*ThreadsY)*sizeof(float));
	
	int nX = M / (BlocksX*ThreadsX);
	int nY = N / (BlocksY*ThreadsY);

 	int stride;

	int k = 1;
  	char *iter_name;
  	iter_name = (char *)malloc(20*sizeof(char));

	do {
    //update red point in gpu
		redUpdate<<<dimGrid, dimBlock>>>(C_gpu, delta_gpu, nX, nY);

    //update black points in gpu
		blackUpdate<<<dimGrid, dimBlock>>>(C_gpu, delta_gpu, nX, nY);

    /*
    find row-wise max changing, store in postion (i, 0) of e_gpu
    stride is number of consecutive points in parallel reduction, doubled each time
    function rowWiseReduction is called until stride == dimension length
    */
    stride = 2;
    do {
      rowWiseReduction<<<dimGrid, dimBlock>>>(delta_gpu, stride);
      stride *= 2;
    } while (stride <= BlocksX*ThreadsX);

    /*
    find gloabal max changing by applying column-wise reduction
    global max is stored in position (0, 0) of e_gpu
    stride is number of consecutive points in parallel reduction, doubled each time
    function columnWiseReduction is called until stride == dimension length
    */
    stride = 2;
    do {
      columnWiseReduction<<<dimGrid, dimBlock>>>(delta_gpu, stride);
      stride *= 2;
    } while (stride <= BlocksY*ThreadsY);

    //send max changing to CPU
    cudaMemcpy(&delta, delta_gpu, sizeof(float), cudaMemcpyDeviceToHost);
    
    //save intermidiate states every 100 iterrations
    if (k % 100 == 0) {
      cudaMemcpy(C, C_gpu, (M*N)*sizeof(float), cudaMemcpyDeviceToHost);

      iter_name = (char *)malloc(50*sizeof(char));
      sprintf(iter_name, "%05d", k);
      strcat(iter_name, ".txt");

      write2File(C, iter_name);
    } 
    k += 1;
	} while (delta > Delta);
	
	//send final state to CPU
	cudaMemcpy(C, C_gpu, (M*N)*sizeof(float), cudaMemcpyDeviceToHost);
	cudaFree(C_gpu);
	
	//save final result
	iter_name = (char *)malloc(50*sizeof(char));
	sprintf(iter_name, "%05d", k);
	strcat(iter_name, ".txt");
	write2File(C, iter_name);
}



void initialize(float *C)
{
    int i, j;
    for (i = 0; i < M; i++)
        for (j = 0; j < N; j++)
        {
            if (i >= (M / 2 - 20) && i < (M / 2 + 20) && j >= (N / 2 - 20) && j < (N / 2 + 20))
                *(C + i * N + j) = 80.0;
            else
                *(C + i * N + j) = 25.0;
        }
}

void write2File(float *C, char name[])
{
    char path[50] = "./results/CUDA/";
    strcat(path, name);

    FILE *result = fopen(path, "w");
    int i, j;

    for (i = 0; i < M; i++)
    {
        for (j = 0; j < N; j++)
        {
            fprintf(result, "%lf\t", *(C + i * N + j));
        }
        fprintf(result, "\n");
    }

    fclose(result);
}

__global__ void redUpdate(float *C_gpu, float *delta_gpu, int nX, int nY) {
	int index_x, start_x, end_x;
	start_x = (blockIdx.x * blockDim.x + threadIdx.x) * nX;
	end_x = (blockIdx.x * blockDim.x + threadIdx.x + 1) * nX;
		
	int index_y, start_y, end_y;
	start_y = (blockIdx.y * blockDim.y + threadIdx.y) * nY;
	end_y = (blockIdx.y * blockDim.y + threadIdx.y + 1) * nY;
	
	float u, d, l, r;
	float local_delta = 0;
	
	for (index_x = start_x; index_x < end_x; index_x ++ ) {
		for (index_y = start_y + ((index_x + start_y) % 2); index_y < end_y; index_y += 2) {
		
			//check boundary condition dim x
			if (index_x == 0) {
				u = U;
				d = *(C_gpu + (index_x + 1) * N + index_y);
			} else if (index_x == M - 1){
				u = *(C_gpu + (index_x - 1) * N + index_y);
				d = D;	
			} else {
				u = *(C_gpu + (index_x-1) * N + index_y);
				d = *(C_gpu + (index_x+1) * N + index_y);
			}
			
			//check boundary condition dim y
			if (index_y == 0) {
				l = L;
				r = *(C_gpu + index_x * N + index_y + 1);
			} else if (index_y == N - 1) {
				l = *(C_gpu + index_x * N + index_y - 1);
				r = R;
			} else {
				l = *(C_gpu + index_x * N + index_y - 1);
				r = *(C_gpu + index_x * N + index_y + 1);
			}
			
			//update local delta
			if (abs(*(C_gpu + index_x * N + index_y) - (u + d + l + r)/4) > local_delta) {
				local_delta = abs(*(C_gpu + index_x * N + index_y) - (u + d + l + r)/4);
			}
			
			//update point value
			*(C_gpu + index_x * N + index_y) = (u + d + l + r)/4;		
		}
	}
	*(delta_gpu + (blockIdx.x*blockDim.x + threadIdx.x)*blockDim.y*BlocksY + blockIdx.y*blockDim.y + threadIdx.y) = local_delta;
}


__global__ void blackUpdate(float *C_gpu, float *delta_gpu, int nX, int nY) {
	int index_x, start_x, end_x;
	start_x = (blockIdx.x * blockDim.x + threadIdx.x) * nX;
	end_x = (blockIdx.x * blockDim.x + threadIdx.x + 1) * nX;
		
	int index_y, start_y, end_y;
	start_y = (blockIdx.y * blockDim.y + threadIdx.y) * nY;
	end_y = (blockIdx.y * blockDim.y + threadIdx.y + 1) * nY;
	
	float u, d, l, r;
	float local_delta = 0;
	
	for (index_x = start_x; index_x < end_x; index_x ++ ) {
		for (index_y = start_y + ((index_x + start_y + 1) % 2); index_y < end_y; index_y += 2) {
		
			//check boundary condition dim x
			if (index_x == 0) {
				u = U;
				d = *(C_gpu + (index_x + 1) * N + index_y);
			} else if (index_x == M - 1){
				u = *(C_gpu + (index_x - 1) * N + index_y);
				d = D;	
			} else {
				u = *(C_gpu + (index_x-1) * N + index_y);
				d = *(C_gpu + (index_x+1) * N + index_y);
			}
			
			//check boundary condition dim y
			if (index_y == 0) {
				l = L;
				r = *(C_gpu + index_x * N + index_y + 1);
			} else if (index_y == N - 1) {
				l = *(C_gpu + index_x * N + index_y - 1);
				r = R;
			} else {
				l = *(C_gpu + index_x * N + index_y - 1);
				r = *(C_gpu + index_x * N + index_y + 1);
			}
			
			//update local delta
			if (abs(*(C_gpu + index_x * N + index_y) - (u + d + l + r)/4) > local_delta) {
				local_delta = abs(*(C_gpu + index_x * N + index_y) - (u + d + l + r)/4);
			}
			
			//update point value
			*(C_gpu + index_x * N + index_y) = (u + d + l + r)/4;	
		}
	}
	if (*(delta_gpu + (blockIdx.x*blockDim.x + threadIdx.x)*blockDim.y + threadIdx.y) < local_delta) {
	  *(delta_gpu + (blockIdx.x*blockDim.x + threadIdx.x)*blockDim.y*BlocksY + blockIdx.y*blockDim.y + threadIdx.y) = local_delta;
	}
}

/*
this function will find row-wise maximun in interval of size stride
maximum of this interval is stored in its first position
*/
__global__ void rowWiseReduction(float *delta_gpu, int stride) {
  int index_y = blockIdx.y*blockDim.y + threadIdx.y;
  int index_x = blockIdx.x*blockDim.x + threadIdx.x;

  if (index_y % stride == 0) {
    if (*(delta_gpu + index_x*blockDim.y*BlocksY + index_y) < *(delta_gpu + index_x*blockDim.y*BlocksY + index_y + stride/2)) {
      *(delta_gpu + index_x*blockDim.y*BlocksY + index_y) = *(delta_gpu + index_x*blockDim.y*BlocksY + index_y + stride/2);
    }
  }
}


/*
this function will find column-wise maximun in interval of size stride
maximum of this interval is stored in its first position
*/
__global__ void columnWiseReduction(float *delta_gpu, int stride) {
  int index_y = blockIdx.y*blockDim.y + threadIdx.y;
  int index_x = blockIdx.x*blockDim.x + threadIdx.x;

  if ((index_x % stride == 0) && (index_y == 0)) {
    if (*(delta_gpu + index_x*blockDim.y*BlocksY + index_y) < *(delta_gpu + (index_x + stride / 2)*blockDim.y*BlocksY + index_y)) {
      *(delta_gpu + index_x*blockDim.y*BlocksY + index_y) = *(delta_gpu + (index_x + stride / 2)*blockDim.y*BlocksY + index_y);
    }
  }
}
