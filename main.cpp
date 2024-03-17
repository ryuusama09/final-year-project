
#include <bits/stdc++.h>
#include <iostream>
#include <cstdio>
#include <fstream>
#include <cstring>

#include "matrix.h"
#include "range.h"
#include "envelope.h"
#include "envelope_tree.h"
#include "tests.h"

using namespace std;
using namespace matrix;
using namespace envelope;

// void printBreakpointList(const vector< Breakpoint > *bp)
// {
//     for (size_t i = 0; i < bp->size(); i++) {
//         cout << '(' << (*bp)[i].row << ',' << (*bp)[i].col << ')';
//     }
// }


void benchmarks(size_t nRows, size_t nCols)
{
    cout << "Benchmarks for " << nRows << " rows and " << nCols << " columns \n\n";
    
    SubmatrixQueriesTest test = SubmatrixQueriesTest(nRows, nCols);
    
    cout << "\n========================================";
    cout << "\nBeginning column queries benchmarks ..." << endl;
    test.multipleBenchmarksColQueries(10000);
    
    cout << "\n========================================";
    cout << "\nBeginning row queries benchmarks ..." << endl;
    test.multipleBenchmarksRowQueries(10000);
    
    cout << "\n========================================";
    cout << "\nBeginning submatrix queries benchmarks ..." << endl;
    test.multipleBenchmarksSubmatrixQueries(1000);
    
}


int main(int argc, const char * argv[])
{
    size_t nRows = 10000; // default values for the number of columns and rows
    size_t nCols = 10000;
    size_t minRows = 0, minCols = 0;
    size_t stepSize = 50;
    size_t samplesPerSize = 50;
    benchmarks(nRows, nCols);
    // cout << "YEA";
    return 0;
    
    // int mode = -1; // for default behavior
    
    // const char* filename = NULL;
    // bool externalOutput = false;

    // if (argc >= 3) {
    //     sscanf(argv[1], "%lu", &nRows);
    //     sscanf(argv[2], "%lu", &nCols);
    // }

    // int i = 3;

    // SubmatrixQueriesTest::benchmarkNaiveQueries = false;
    // SubmatrixQueriesTest::verboseBenchmarks = false;
    // SubmatrixQueriesTest::verboseMatrixGeneration = false;
    
    // while (i < argc) {
    //     if (strcmp("-o", argv[i]) == 0) {
    //         assert(i+1 < argc);
    //         filename = argv[i+1];
    //         externalOutput = true;
    //         i = i+2;
    //     }else if (strcmp("--minSize", argv[i]) == 0){
    //         assert(i+2 < argc);
    //         sscanf(argv[i+1], "%lu", &minRows);
    //         sscanf(argv[i+2], "%lu", &minCols);
    //         i = i+3;
    //     }else if (strcmp("--step", argv[i]) == 0){
    //         assert(i+1 < argc);
    //         sscanf(argv[i+1], "%lu", &stepSize);
    //         i = i+2;
    //     }else if (strcmp("--samples-per-size", argv[i]) == 0){
    //         assert(i+1 < argc);
    //         sscanf(argv[i+1], "%lu", &samplesPerSize);
    //         i = i+2;
    //     }else if (strcmp("--mode", argv[i]) == 0){
    //         assert(i+1 < argc);
    //         sscanf(argv[i+1], "%d", &mode);
    //         i = i+2;
    //     }else if (strcmp("--naive", argv[i]) == 0){
    //         i = i+1;
    //         SubmatrixQueriesTest::benchmarkNaiveQueries = true;
    //     }else if (strcmp("-v", argv[i]) == 0 || strcmp("--verbose", argv[i]) == 0){
    //         i = i+1;
    //         SubmatrixQueriesTest::verboseBenchmarks = true;
    //         SubmatrixQueriesTest::verboseMatrixGeneration = true;
    //     }
    // }

    
    
    // switch (mode) {
    //     case 0:
    //         SubmatrixQueriesTest::verboseMatrixGeneration = true;
    //         SubmatrixQueriesTest::verboseBenchmarks = true;
    //         testInitalization(nRows,nCols);
    //         break;
            
    //     case 1:
    //         SubmatrixQueriesTest::verboseBenchmarks = true;
    //         SubmatrixQueriesTest::verboseMatrixGeneration = true;
    //         testTest(nRows,nCols);
    //         break;

    //     case 2:
    //         multiSizePositionQueriesBenchmarks(nRows, nCols, ((float)nRows)/((float) stepSize),samplesPerSize);
    //         break;

    //     case 4:
    //         multiSizeFastestQueriesBenchmarks(nRows, nCols, minRows, minCols,stepSize,samplesPerSize, 1000,100, filename);
    //         break;
        
    //     case 5:
    //         SubmatrixQueriesTest::verboseBenchmarks = true;
    //         SubmatrixQueriesTest::verboseMatrixGeneration = true;
    //         benchmarks(nRows, nCols);
    //         break;
    //     case 6:
    //         multiSizeAllQueriesBenchmarks(nRows, nCols, minRows, minCols,stepSize,samplesPerSize,filename);
    //         break;
            
    //     case 7:
    //         envelopeSizeStatistics(max(nRows,nCols),min(minRows,minCols),stepSize,true,samplesPerSize,filename);
    //         break;
            
    //     case 3:
    //     default:
    //         if (externalOutput) {
    //             multiSizeSubmatrixQueriesBenchmarks(nRows, nCols, minRows, minCols,stepSize,samplesPerSize,filename);
    //         }else{
    //             multiSizeSubmatrixQueriesBenchmarks(nRows, nCols, minRows, minCols,stepSize,samplesPerSize);
    //         }
            
    //         break;
    // }
    

    // return 0;
}
