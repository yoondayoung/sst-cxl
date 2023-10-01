#include <stdio.h>
#include <stdlib.h>

extern "C" 
void ariel_enable() { printf("Inside Ariel\n"); }

int main(int argc, char* argv[]) {
    const int LENGTH = 3000;

	ariel_enable();

    printf("Allocating arrays of size %d elements.\n", LENGTH);
    double* a = (double*) malloc(sizeof(double) * LENGTH);
    double* b = (double*) malloc(sizeof(double) * LENGTH);
    
    printf("Done allocating arrays.\n");

     int i;
        for(i = 0; i < LENGTH; ++i) {
                a[i] = i;
                b[i] = LENGTH - i;
        }


        printf("Perfoming the fast_c compute loop...\n");
        #pragma omp parallel num_threads(2)
        for(i = 0; i < LENGTH; ++i) {
                //printf("issuing a write to: %llu (fast_c)\n", ((unsigned long long int) &fast_c[i]));
                b[i] = 2.0 * a[i] + 1.5;
        }

        double sum = 0;
        for(i = 0; i < LENGTH; ++i) {
                sum += b[i];
        }

        printf("Sum of arrays is: %f\n", sum);
        printf("Freeing arrays...\n");

        free(a);
        free(b);

        printf("Done.\n");
}
