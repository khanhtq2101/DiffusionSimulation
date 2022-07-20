#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>

#define m 256
#define n 256

#define U 100
#define D 100
#define L 0
#define R 0

void Initialize(float *C)
{
    int i, j;
    for (i = 0; i < m; i++)
        for (j = 0; j < n; j++)
        {
            if (i >= (m / 2 - 20) && i < (m / 2 + 20) && j >= (n / 2 - 20) && j < (n / 2 + 20)) {
                *(C + i * n + j) = 80.0;
               
            } else {
                *(C + i * n + j) = 25.0;
            }
        }
}


void redUpdate(float *C) {
	int i, j;
	float u, d, l, r;
	for (i = 0; i < n; i++) {
		for (j = i % 2; j < m; j += 2){
			if (i == 0) {
				u = U;
				d = *(C +(i+1)*n + j);
			} else if (i == m -1){
				u = *(C +(i-1)*n + j);
				d = D;	
			} else {
				u = *(C +(i-1)*n + j);
				d = *(C +(i+1)*n + j);
			}
			
			if (j == 0) {
				l = L;
				r = *(C +i*n + j + 1);
			} else if (j == n-1) {
				l = *(C +i*n + j);
				r = R;
			} else {
				l = *(C +i*n + j - 1);
				r = *(C +i*n + j + 1);
			}
			*(C + i*n + j) = (u + d + l + r)/4;
		} 
	}
}

void blackUpdate(float *C) {
	int i, j;
	float u, d, l, r;
	for (i = 0; i < n; i++) {
		for (j = (i+1) % 2; j < m; j += 2){
			if (i == 0) {
				u = U;
				d = *(C +(i+1)*n + j);
			} else if (i == m -1){
				u = *(C +(i-1)*n + j);
				d = D;	
			} else {
				u = *(C +(i-1)*n + j);
				d = *(C +(i+1)*n + j);
			}
			
			if (j == 0) {
				l = L;
				r = *(C +i*n + j + 1);
			} else if (j == n-1) {
				l = *(C +i*n + j);
				r = R;
			} else {
				l = *(C +i*n + j - 1);
				r = *(C +i*n + j + 1);
			}
			*(C + i*n + j) = (u + d + l + r)/4;
		} 
	}
}

void Write2File(float *C, char name[])
{
    char path[50] = "./results/Linear/";
    strcat(path, name);

    FILE *result = fopen(path, "a");
    int i, j;

    for (i = 0; i < m; i++)
    {
        for (j = 0; j < n; j++)
        {
            fprintf(result, "%lf\t", *(C + i * n + j));
        }
        fprintf(result, "\n");
    }

    fclose(result);
}

int main(int argc, char *argv[]) {

	float *C;
	C = (float *)malloc((m * n) * sizeof(float));
	Initialize(C);
	Write2File(C, "origin.txt");
	
	int i, j;
	float u, d, l, r;
	int k;
	for (k = 0; k < 10000; k++) {
		redUpdate(C);
		blackUpdate(C);

	}
	//DisplayMatrix(C, m, n);
	Write2File(C, "result_linear.txt");
}
