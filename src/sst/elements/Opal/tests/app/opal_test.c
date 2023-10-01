// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <stdio.h>
#include <stdlib.h>

extern "C" {
        void ariel_enable() { printf("Inside Ariel"); }
        void* mlm_malloc(size_t size, int level)
{
	if(size == 0)
      {
		printf("ZERO BYTE MALLOC\n");
		void* bt_entries[64];
		exit(-1);
	}

	printf("Performing a mlm Malloc for size %llu\n", size);

	return malloc(size);
}
}


int main(int argc, char* argv[]) {

        const int LENGTH = 2000;

	ariel_enable();

        printf("Allocating arrays of size %d elements.\n", LENGTH);
        // weight_pre_malloc();
        double* a = (double*) mlm_malloc(sizeof(double) * LENGTH, 1);
        // double* a = (double*) malloc(sizeof(double) * LENGTH);
        double* b = (double*) mlm_malloc(sizeof(double) * LENGTH, 1);
        double* c = (double*) malloc(sizeof(double) * LENGTH);
        // double* c = (double*) malloc(sizeof(double) * LENGTH);
        // weight_post_malloc();
        
        printf("allocated address: a:%x b:%x c:%x\n", a, b, c);
        printf("Done allocating arrays.\n");

        int i;
        for(i = 0; i < LENGTH; ++i) {
                a[i] = i;
                b[i] = LENGTH - i;
                c[i] = 0;
        }
       

        // printf("Perfoming the fast_c compute loop...\n");
        // #pragma omp parallel num_threads(2)
        for(i = 0; i < LENGTH; ++i) {
                //printf("issuing a write to: %llu (fast_c)\n", ((unsigned long long int) &fast_c[i]));
                c[i] = 2.0 * a[i] + 1.5 * b[i];
        }

        double sum = 0;
        for(i = 0; i < LENGTH; ++i) {
                sum += c[i];
        }

        printf("Sum of arrays is: %f\n", sum);
        // printf("Freeing arrays...\n");

        // free(a);
        // free(b);
        // free(c);

        for (int ii=0; ii<10000; ii++);

        printf("Done.\n");
}
