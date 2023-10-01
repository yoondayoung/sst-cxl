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
#include "/home/ydy/scratch/src/pin-3.22-98547-g7a303a835-gcc-linux/source/include/pin/pin.H"
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

ofstream TraceLog;

extern "C"
void ariel_enable() { printf("Inside Ariel\n"); }

void Fini(int code, void *v)
{
    TraceLog << endl << "#eof..." << endl;
 
    if (TraceLog.is_open()) TraceLog.close();
}
 

int main(int argc, char* argv[]) {
        PIN_InitSymbols();
        if (PIN_Init(argc, argv)) return -1;
        
        TraceLog.open("MemTrace_Log.txt", ofstream::out);
        TraceLog << "### Memory Trace Log ###" << endl << endl;

        const int LENGTH = 2000;

	ariel_enable();

        printf("Allocating arrays of size %d elements.\n", LENGTH);
        double* a = (double*) malloc(sizeof(double) * LENGTH);
        double* b = (double*) malloc(sizeof(double) * LENGTH);
        double* c = (double*) malloc(sizeof(double) * LENGTH);
        printf("Done allocating arrays.\n");

        int i;
        for(i = 0; i < LENGTH; ++i) {
                a[i] = i;
                b[i] = LENGTH - i;
                c[i] = 0;
        }


        printf("Perfoming the fast_c compute loop...\n");
        #pragma omp parallel num_threads(2)
        for(i = 0; i < LENGTH; ++i) {
                //printf("issuing a write to: %llu (fast_c)\n", ((unsigned long long int) &fast_c[i]));
                c[i] = 2.0 * a[i] + 1.5 * b[i];
        }

        double sum = 0;
        for(i = 0; i < LENGTH; ++i) {
                sum += c[i];
        }

        printf("Sum of arrays is: %f\n", sum);
        printf("Freeing arrays...\n");

        free(a);
        free(b);
        free(c);

        printf("Done.\n");

        PIN_AddFiniFunction(Fini, NULL);
        PIN_StartProgram();
}
