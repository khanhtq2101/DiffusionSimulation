#include <stdio.h>
#include <mpi.h>
#include <stdlib.h>
#include <string.h>

#define M 256
#define N 256

#define U 100
#define D 0
#define L 0
#define R 100

#define Delta 0.1

void initialize(float *C);
void write2File(float *C, char name[]);
void redUpdate(float *C_local, float *Up, float *Down, float *delta, int h, int w, int id, int size);
void blackUpdate(float *C_local, float *Up, float *Down, float *delta, int h, int w, int id, int size);


int main(int argc, char *argv[]) {
	int id, size;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &id);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	
	int m = M / size;
	float *C_local, *C;
	C = (float *) malloc((M * N) * sizeof(float));
	C_local = (float *) malloc((m*N) * sizeof(float));

	
	//Processor 1 is responsibale to initialize grid value
	if (id == 1) {
		printf("I am processor %d\n", id);
		printf("Number of processor: %d \n", size); 
		initialize(C);
	}
	
	//Distribute computational grid to all processors
	MPI_Scatter(C, m*N, MPI_FLOAT, C_local, m*N, MPI_FLOAT, 1, MPI_COMM_WORLD);	
	
	//Up and Down of local compputational grid
	int i;
	float *Up, *Down;
	Up = (float *) malloc(N * sizeof(float));
	Down = (float *) malloc(N * sizeof(float));
	
	//Up and down boundary of first and last processor
	if (id == 0) {
		for (i = 0; i < N; i++){
			Up[i] = U;
		}
	} else if (id == size - 1){
		for (i = 0; i < N; i ++){
			Down[i] = D;
		}
	}
	
	//maximum changing delta
	float delta, delta_glob;

	do {
		delta = 0;
		
		//Communication 
		if (id == 0){
			//Send down to next processor
			MPI_Send(C_local + (m-1)*N, N, MPI_FLOAT, id + 1, 1, MPI_COMM_WORLD);
			
			//Receive Down from next processor
			MPI_Recv(Down, N, MPI_FLOAT, id + 1, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);	
		} else if (id == size - 1) {
			//Send Up to previous processor
			MPI_Send(C_local, N, MPI_FLOAT, id - 1, 2, MPI_COMM_WORLD);
			
			//Receive Up from previous processor
			MPI_Recv(Up, N, MPI_FLOAT, id - 1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);		
		} else {
			//Send Up to previous processor
			MPI_Send(C_local, N, MPI_FLOAT, id - 1, 2, MPI_COMM_WORLD);
			//Send down to next processor
			MPI_Send(C_local + (m-1)*N, N, MPI_FLOAT, id + 1, 1, MPI_COMM_WORLD);
			
			//Receive Up from previous processor
			MPI_Recv(Up, N, MPI_FLOAT, id - 1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			//Receive Down from next processor
			MPI_Recv(Down, N, MPI_FLOAT, id + 1, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		}
		
		//Red and black update
		redUpdate(C_local, Up, Down, &delta, m, N, id, size);
		blackUpdate(C_local, Up, Down, &delta, m, N, id, size);
		
		//check stopping condition
		//Obtain global maximum changing and braodcast to all process
		MPI_Allreduce(&delta, &delta_glob, 1, MPI_FLOAT, MPI_MAX, MPI_COMM_WORLD);

	} while (delta_glob > Delta);
	
	printf("Delta: %f \n", delta);
	
	//Gather heat grid to processor 0
	MPI_Gather(C_local, m*N, MPI_FLOAT, C, m*N, MPI_FLOAT, 0, MPI_COMM_WORLD);
		
	if (id == 0){
		printf("Detla max: %f \n", delta_glob);
		printf("Value in top right: %f\n", C[N-1]);
		write2File(C, "final.txt");
	}
		
	MPI_Finalize();
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

void redUpdate(float *C_local, float *Up, float *Down, float *delta, int h, int w, int id, int size) {
		int red_align, i, j;
		if (h % 2 == 0) {
			red_align = 0;
		} else {
			red_align = id %2;
		}
		
		//neighboring points
		float u, d, l, r;
		
		//changing of value
		float c;
		
		for (i = 0; i < h; i ++) {
			for (j = (i + red_align) % 2; j < w; j += 2) {
				//check boundary condition dim x
				if (i == 0) {
					u = Up[j];
					d = C_local[(i+1)*w + j];
				} else if (i == h - 1) {
					u = C_local[(i-1)*w +j];
					d = Down[j];
				} else {
					u = C_local[(i-1)*w +j];
					d = C_local[(i+1)*w + j];
				}
				
				//check boundary condition dim y
				if (j == 0) {
					r = C_local[i*w + j + 1];
					l = L;
				} else if (j == w - 1) {
					r = R;
					l = C_local[i*w + j - 1];
				} else {
					l = C_local[i*w + j - 1];
					r = C_local[i*w + j + 1];
				}
				
				c = C_local[i*w + j] - (u + d + l + r)/4;
				if (c > *delta) {
					*delta = c;
				}
				
				//Update center value
				C_local[i*w + j] = (u + d + l + r)/4;
			}
		}
}

void blackUpdate(float *C_local, float *Up, float *Down, float *delta, int h, int w, int id, int size) {
		int black_align, i, j;
		if (h % 2 == 0) {
			black_align = 0;
		} else {
			black_align = id %2;
		}
		
		//neighboring points
		float u, d, l, r;
		
		//changing of value
		float c;
		
		for (i = 0; i < h; i ++) {
			for (j = (i + black_align + 1) % 2; j < w; j += 2) {
				//check boundary condition vertically
				if (i == 0) {
					u = Up[j];
					d = C_local[(i+1)*w + j];
				} else if (i == h - 1) {
					u = C_local[(i-1)*w +j];
					d = Down[j];
				} else {
					u = C_local[(i-1)*w +j];
					d = C_local[(i+1)*w + j];
				}
				
				//check boundary condition horizontally
				if (j == 0) {
					r = C_local[i*w + j + 1];
					l = L;
				} else if (j == w-1) {
					r = R;
					l = C_local[i*w + j - 1];
				} else {
					l = C_local[i*w + j - 1];
					r = C_local[i*w + j + 1];
				}
								
				c = C_local[i*w + j] - (u + d + l + r)/4;
				if (c > *delta) {
					*delta = c;
				}

				//Update center value
				C_local[i*w + j] = (u + d + l + r)/4;
			}
		}		
}

void write2File(float *C, char name[])
{
    char path[50] = "./results/MPI/";
    strcat(path, name);

    FILE *result = fopen(path, "a");
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
