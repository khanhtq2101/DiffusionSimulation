#include <stdio.h>
#include <mpi.h>
#include <stdlib.h>


#define M 256
#define N 256

void initialize(float *C);
void write2File(float *C, char name[]);


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
	printf("I am processor %d %f \n", id, C_local[128]);
		
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
