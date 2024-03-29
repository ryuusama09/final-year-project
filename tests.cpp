

#include "tests.h"
#include <Windows.h>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <climits>
#include <set>
#include <cfloat>
#include <time.h>
#include <fstream>
#include <thread>
#include "debug_assert.h"
#include "oracle_monge_matrix.h"
#define BILLION 1000000000
extern "C++"
{
#ifdef _WIN32
//#include <pthread.h>
#else
#include <pthread.h>
#endif
}

double fRand(double fMin, double fMax)
{
    double f = (double)rand() / RAND_MAX;
    return fMin + f * (fMax - fMin);
}


#ifdef __MACH__
#define ZeroTime 0
#else
#define ZeroTime {0}
#endif

timespec diffTS(timespec start, timespec end)
{
	timespec temp;
	if ((end.tv_nsec-start.tv_nsec)<0) {
		temp.tv_sec = end.tv_sec-start.tv_sec-1;
		temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
	} else {
		temp.tv_sec = end.tv_sec-start.tv_sec;
		temp.tv_nsec = end.tv_nsec-start.tv_nsec;
	}
	return temp;
}

timespec addTS(timespec t1, timespec t2)
{
	timespec temp;
	temp.tv_sec = t1.tv_sec + t2.tv_sec;
	temp.tv_nsec = t1.tv_nsec + t2.tv_nsec;
	if(temp.tv_nsec >= 1000000000){
		temp.tv_sec++;
		temp.tv_nsec -= 1000000000;
	}
	return temp;
}
int clock_gettime(int /*clk_id*/, struct timespec *ts) {
    LARGE_INTEGER counter;
    static LARGE_INTEGER frequency;
    static BOOL init = QueryPerformanceFrequency(&frequency);

    if (!init) {
        // Failed to initialize frequency
        return -1;
    }

    if (!QueryPerformanceCounter(&counter)) {
        // Failed to get performance counter
        return -1;
    }

    // Calculate seconds and nanoseconds
    ts->tv_sec = counter.QuadPart / frequency.QuadPart;
    ts->tv_nsec = ((counter.QuadPart % frequency.QuadPart) * BILLION) / frequency.QuadPart;

    return 0;
}
bench_time_t diff(bench_time_t t1, bench_time_t t2)
{
#ifdef __MACH__
    return t1 - t2;
#else
    return diffTS(t2,t1);
#endif
}

bench_time_t add(bench_time_t t1, bench_time_t t2)
{
#ifdef __MACH__
    return t1 + t2;
#else
    return addTS(t2,t1);
#endif
}

bench_time_t now()
{
#ifdef __MACH__
    return clock();
#else
    timespec t;
    clock_gettime(0,&t);
    return t;
#endif
}

double timespecAsSeconds(timespec ts)
{
    return ts.tv_sec + (((double)ts.tv_nsec)/((double)1000000000));
}

double timespecAsMiliSeconds(timespec ts)
{
    return 1000*ts.tv_sec + (((double)ts.tv_nsec)/((double)1000000));
}

double benchTimeAsMiliSeconds(bench_time_t t)
{
#ifdef __MACH__
    return 1000*((double)t)/CLOCKS_PER_SEC;
#else
    return timespecAsMiliSeconds(t);
#endif
}

#define PRINT_TEST_MATRIX false

#define USE_ORACLE_MATRIX true
#define MULTITHREAD_GENERATION true
#define GENERATION_THREAD_COUNT 15

bool SubmatrixQueriesTest::benchmarkNaiveQueries = false;
bool SubmatrixQueriesTest::showProgressBar = true;
bool SubmatrixQueriesTest::verboseBenchmarks = true;
bool SubmatrixQueriesTest::verboseMatrixGeneration = true;

SubmatrixQueriesTest::SubmatrixQueriesTest(Matrix<double> *m)
{
    DEBUG_ASSERT(m->isInverseMonge());
    // copy the matrix
    _testMatrix = new ComplexMatrix<double>(m);
    bench_time_t time = now();
 
    _queryDS = new SubmatrixQueriesDataStructure<double>(_testMatrix);

    if (SubmatrixQueriesTest::verboseBenchmarks) {
        time = diff(now(),time);
        cout << "Building Data Structure: " << benchTimeAsMiliSeconds(time) << " ms" << endl;
    }
}

SubmatrixQueriesTest::SubmatrixQueriesTest(size_t rows, size_t cols)
{
    bench_time_t time = now();

#if USE_ORACLE_MATRIX
    _testMatrix = new OracleMongeMatrix<double>(rows,cols);
#elif MULTITHREAD_GENERATION
    _testMatrix = generateInverseMongeMatrixSlopeMultithread(rows, cols,GENERATION_THREAD_COUNT);
#else
    _testMatrix = generateInverseMongeMatrixSlope(rows, cols);
#endif

    if (SubmatrixQueriesTest::verboseBenchmarks) {
        time = diff(now(),time);
        cout << "Building Matrix: " << benchTimeAsMiliSeconds(time) << " ms" << endl;
        time = now();
    }
//    DEBUG_ASSERT(_testMatrix->isInverseMonge());
    
    _queryDS = new SubmatrixQueriesDataStructure<double>(_testMatrix);
    if (SubmatrixQueriesTest::verboseBenchmarks) {
        time = diff(now(),time);
        cout << "Building Data Structure: " << benchTimeAsMiliSeconds(time) << " ms" << endl;
        cout << "Max/min envelope size for rows: " << _queryDS->rowsTree()->maxEnvelopeSize() << " / "<< _queryDS->rowsTree()->minEnvelopeSize() << endl;
        cout << "Max/min envelope size for cols: " << _queryDS->columnTree()->maxEnvelopeSize()  << " / "<< _queryDS->columnTree()->minEnvelopeSize() << endl;
    }
    
#if PRINT_TEST_MATRIX
    cout << "\n";
    _testMatrix->print();
#endif
    
    if (SubmatrixQueriesTest::verboseBenchmarks) {
         cout << "\n";
    }
}

SubmatrixQueriesTest::~SubmatrixQueriesTest()
{
    delete _testMatrix;
    delete _queryDS;
}

bool SubmatrixQueriesTest::testColumnQuery(Range rowRange, size_t col, bench_time_t *naiveTime, bench_time_t *queryTime)
{
    double naiveMax, queryMax;
    bench_time_t clock1, clock2, clock3;

    clock1 = now();
    queryMax = _queryDS->rowsTree()->maxForColumnInRange(col, rowRange.min, rowRange.max);
    clock2 = now();
    
    naiveMax = SubmatrixQueriesTest::naiveMaximumInColumn(_testMatrix, rowRange, col);
    
    clock3 = now();
    
    if (naiveTime) {
        *naiveTime = add(*naiveTime,diff(clock3,clock2));
    }
    if (queryTime) {
        *queryTime = add(*queryTime,diff(clock2,clock1));
    }
    
    return queryMax == naiveMax;
}

bool SubmatrixQueriesTest::testColumnQuery(bench_time_t *naiveTime, bench_time_t *queryTime)
{
    size_t col = rand() % (_testMatrix->cols());
    
    size_t r1, r2;
    r1 = rand() % (_testMatrix->rows());
    r2 = rand() % (_testMatrix->rows());
    
    Range r = Range(min(r1,r2),max(r1,r2));
    
    return testColumnQuery(r, col, naiveTime, queryTime);
}

bool SubmatrixQueriesTest::testCascadingColQuery(Range rowRange, size_t col, bench_time_t *queryTime, bench_time_t *cascadingTime, bench_time_t *simpleCascadingTime)
{
    double cascadingMax, simpleCascadingMax, queryMax;
    bench_time_t clock1, clock2, clock3, clock4;
    
    clock1 = now();
    cascadingMax = _queryDS->rowsTree()->cascadingMaxInRange(col, rowRange);
    
    clock2 = now();
    simpleCascadingMax = _queryDS->rowsTree()->simpleCascadingMaxInRange(col, rowRange);
    
    clock3 = now();
    queryMax = _queryDS->rowsTree()->maxForColumnInRange(col, rowRange.min, rowRange.max);
    
    clock4 = now();
    
    if (queryTime) {
        *queryTime = add(*queryTime,diff(clock4, clock3));
    }
    if (cascadingTime) {
        *cascadingTime = add(*cascadingTime,diff(clock2, clock1));
    }
    if (simpleCascadingTime) {
        *simpleCascadingTime = add(*simpleCascadingTime,diff(clock3, clock2));
    }
    
    if (queryMax != cascadingMax) {
        cout << "Test failed: "<<endl;
        cout << "\tquery ranges : row = (" << rowRange.min <<","<<rowRange.max<<")";
        cout << " col = " << col <<endl;
        cout << "\tqueryMax: " << queryMax;
        cout << " ; cascading: " << cascadingMax << endl;
        
    }

    return (queryMax == cascadingMax) && (queryMax == simpleCascadingMax);
}

bool SubmatrixQueriesTest::testCascadingColQuery(bench_time_t *queryTime, bench_time_t *cascadingTime, bench_time_t *simpleCascadingTime)
{
    size_t col = rand() % (_testMatrix->cols());
    
    size_t r1,r2;
    r1 = rand() % (_testMatrix->rows());
    r2 = rand() % (_testMatrix->rows());
    
    Range r = Range(min(r1,r2),max(r1,r2));
    
    return testCascadingColQuery(r, col,queryTime,cascadingTime,simpleCascadingTime);
}


bool SubmatrixQueriesTest::testRowQuery(Range colRange, size_t row, bench_time_t *naiveTime, bench_time_t *queryTime)
{
    double naiveMax, queryMax;
    bench_time_t clock1, clock2, clock3;
    
    clock1 = now();
    queryMax = _queryDS->columnTree()->maxForRowInRange(row, colRange.min, colRange.max);
    
    clock2 = now();
    naiveMax = SubmatrixQueriesTest::naiveMaximumInRow(_testMatrix, colRange, row);
    
    clock3 = now();

    if (naiveTime) {
        *naiveTime = add(*naiveTime,diff(clock3,clock2));
    }
    if (queryTime) {
        *queryTime = add(*queryTime,diff(clock2,clock1));
    }

    return queryMax == naiveMax;
}

bool SubmatrixQueriesTest::testRowQuery(bench_time_t *naiveTime, bench_time_t *queryTime)
{
    size_t row = rand() % (_testMatrix->rows());
    
    size_t c1,c2;
    c1 = rand() % (_testMatrix->cols());
    c2 = rand() % (_testMatrix->cols());
    
    Range c = Range(min(c1,c2),max(c1,c2));
    
    return testRowQuery(c, row,naiveTime,queryTime);
}

bool SubmatrixQueriesTest::testCascadingRowQuery(Range colRange, size_t row, bench_time_t *queryTime, bench_time_t *cascadingTime, bench_time_t *simpleCascadingTime)
{
    double cascadingMax, queryMax, simpleCascadingMax;
    bench_time_t clock1, clock2, clock3, clock4;
    
    clock1 = now();
    cascadingMax = _queryDS->columnTree()->cascadingMaxInRange(row, colRange);
    
    clock2 = now();
    simpleCascadingMax = _queryDS->columnTree()->simpleCascadingMaxInRange(row, colRange);

    clock3 = now();
    queryMax = _queryDS->columnTree()->maxForRowInRange(row, colRange.min, colRange.max);

    clock4 = now();

    if (queryTime) {
        *queryTime = add(*queryTime,diff(clock4, clock3));
    }
    if (cascadingTime) {
        *cascadingTime = add(*cascadingTime,diff(clock2, clock1));
    }
    if (simpleCascadingTime) {
        *simpleCascadingTime = add(*simpleCascadingTime,diff(clock3, clock2));
    }

    return (queryMax == cascadingMax) && (queryMax == simpleCascadingMax);
}

bool SubmatrixQueriesTest::testCascadingRowQuery(bench_time_t *queryTime, bench_time_t *cascadingTime, bench_time_t *simpleCascadingTime)
{
    size_t row = rand() % (_testMatrix->rows());
    
    size_t c1,c2;
    c1 = rand() % (_testMatrix->cols());
    c2 = rand() % (_testMatrix->cols());
    
    Range c = Range(min(c1,c2),max(c1,c2));
    
    return testCascadingRowQuery(c, row,queryTime,cascadingTime,simpleCascadingTime);
}

bool SubmatrixQueriesTest::testSubmatrixQuery(Range rowRange, Range colRange, bench_time_t *naiveTime, bench_time_t *explicitNodesTime, bench_time_t *implicitNodesTime)
{
    double naiveMax, explicitNodesMax, implicitNodesMax;
    
    bench_time_t clock1, clock2, clock3, clock4;
    
    clock1 = now();
    explicitNodesMax = _queryDS->maxInRange(rowRange,colRange);
    clock2 = now();
    naiveMax = SubmatrixQueriesTest::naiveMaximumInSubmatrix(_testMatrix, rowRange, colRange);
    clock3 = now();
    implicitNodesMax = _queryDS->maxInSubmatrix(rowRange,colRange);
    clock4 = now();

    if (naiveTime) {
        *naiveTime = add(*naiveTime,diff(clock3, clock2));
    }
    if (explicitNodesTime) {
        *explicitNodesTime = add(*explicitNodesTime,diff(clock2, clock1));
    }
    if (implicitNodesTime) {
        *implicitNodesTime = add(*implicitNodesTime,diff(clock4, clock3));
    }
    
    if (explicitNodesMax != naiveMax) {
        cout << "Test failed: "<<endl;
        cout << "\tquery ranges : row = (" << rowRange.min <<","<<rowRange.max<<")";
        cout << " col = (" << colRange.min <<","<<colRange.max<<")"<<endl;
        cout << "\tqueryMax: " << explicitNodesMax;
        cout << " ; naiveMax: " << naiveMax << endl;
        
    }
    return (explicitNodesMax == naiveMax) && (implicitNodesMax == naiveMax);
}

bool SubmatrixQueriesTest::testSubmatrixQuery(bench_time_t *naiveTime, bench_time_t *explicitNodesTime, bench_time_t *implicitNodesTime)
{
    size_t r1, r2;
    r1 = rand() % (_testMatrix->rows());
    r2 = rand() % (_testMatrix->rows());
    
    size_t c1,c2;
    c1 = rand() % (_testMatrix->cols());
    c2 = rand() % (_testMatrix->cols());

    
    Range r = Range(min(r1,r2),max(r1,r2));
    Range c = Range(min(c1,c2),max(c1,c2));
    
    return testSubmatrixQuery(r, c, naiveTime, explicitNodesTime,implicitNodesTime);
}

bool SubmatrixQueriesTest::multipleColumnQueryTest(size_t n)
{
    bool result = true;
    bench_time_t naiveTime = ZeroTime, queryTime = ZeroTime;
    
    for (size_t i = 0; i < n && result; i++) {
        result = result && testColumnQuery(&naiveTime, &queryTime);
    }
    if (SubmatrixQueriesTest::verboseBenchmarks) {
        cout << "Benchmark for " << n << " column queries:" <<endl;
        cout << "Naive algorithm: " << benchTimeAsMiliSeconds(naiveTime) << " ms" << endl;
        cout << "Submatrix queries: " << benchTimeAsMiliSeconds(queryTime) << " ms" << endl;
    }
    return result;
}

bool SubmatrixQueriesTest::multipleRowQueryTest(size_t n)
{
    bool result = true;
    bench_time_t naiveTime = ZeroTime, queryTime = ZeroTime;
    
    for (size_t i = 0; i < n && result; i++) {
        result = result && testRowQuery(&naiveTime, &queryTime);
    }

    if (SubmatrixQueriesTest::verboseBenchmarks) {
        cout << "Benchmark for " << n << " row queries:" <<endl;
        cout << "Naive algorithm: " << benchTimeAsMiliSeconds(naiveTime) << " ms" << endl;
        cout << "Submatrix queries: " << benchTimeAsMiliSeconds(queryTime) << " ms" << endl;
    }
    return result;
}

bool SubmatrixQueriesTest::multipleRowQueryTestVsCascading(size_t n)
{
    bool result = true;
    bench_time_t queryTime = ZeroTime, cascadingTime = ZeroTime, simpleCascadingTime = ZeroTime;
    
    for (size_t i = 0; i < n && result; i++) {
        result = result && testCascadingRowQuery(&queryTime, &cascadingTime, &simpleCascadingTime);
    }
    
    if (SubmatrixQueriesTest::verboseBenchmarks) {
        cout << "Benchmark for " << n << " row queries:" <<endl;
        cout << "Submatrix queries: " << benchTimeAsMiliSeconds(queryTime) << " ms" << endl;
        cout << "Simple Cascading queries: " << benchTimeAsMiliSeconds(simpleCascadingTime) << " ms" << endl;
        cout << "Cascading queries: " << benchTimeAsMiliSeconds(cascadingTime) << " ms" << endl;
    }
    return result;
}


bool SubmatrixQueriesTest::multipleColQueryTestVsCascading(size_t n)
{
    bool result = true;
    bench_time_t queryTime = ZeroTime, cascadingTime = ZeroTime, simpleCascadingTime = ZeroTime;
    
   
    for (size_t i = 0; i < n && result; i++) {
        result = result && testCascadingColQuery(&queryTime, &cascadingTime,  &simpleCascadingTime);
    }
    
    if (SubmatrixQueriesTest::verboseBenchmarks) {
        cout << "Cascading Benchmark for " << n << " col queries:" <<endl;
        cout << "Submatrix queries: " << benchTimeAsMiliSeconds(queryTime) << " ms" << endl;
        cout << "Simple Cascading queries: " << benchTimeAsMiliSeconds(simpleCascadingTime) << " ms" << endl;
        cout << "Cascading queries: " << benchTimeAsMiliSeconds(cascadingTime) << " ms" << endl;
    }
    return result;
}

bool SubmatrixQueriesTest::multipleSubmatrixQueryTest(size_t n)
{
    bool result = true;
    bench_time_t naiveTime = ZeroTime, explicitNodesTime = ZeroTime, implicitNodesTime = ZeroTime;
    
    for (size_t i = 0; i < n && result; i++) {
        result = result && testSubmatrixQuery(&naiveTime, &explicitNodesTime, &implicitNodesTime);
    }
    
    if (SubmatrixQueriesTest::verboseBenchmarks) {
        cout << "Cascading Benchmark for " << n << " submatrix queries:" <<endl;
        cout << "Naive algorithm: " << benchTimeAsMiliSeconds(naiveTime) << " ms" << endl;
        cout << "Submatrix queries (explicit nodes): " << benchTimeAsMiliSeconds(explicitNodesTime) << " ms" << endl;
        cout << "Submatrix queries (implicit nodes): " << benchTimeAsMiliSeconds(implicitNodesTime) << " ms" << endl;
    }
    return result;
}

void SubmatrixQueriesTest::multipleBenchmarksRowQueries(size_t n)
{
    bench_time_t naiveTime = ZeroTime, queryTime = ZeroTime, cascadingTime = ZeroTime, simpleCascadingTime = ZeroTime;
    
    multipleBenchmarksRowQueries(n,&naiveTime, &queryTime, &cascadingTime,  &simpleCascadingTime);

    cout << "Benchmark for " << n << " row queries:" <<endl;
    cout << "Naive algorithm: " << benchTimeAsMiliSeconds(naiveTime) << " ms" << endl;
    cout << "Canonical nodes: " << benchTimeAsMiliSeconds(queryTime) << " ms" << endl;
    cout << "Cascading: " << benchTimeAsMiliSeconds(cascadingTime) << " ms" << endl;
    cout << "Simple Cascading queries: " << benchTimeAsMiliSeconds(simpleCascadingTime) << " ms" << endl;
}

void SubmatrixQueriesTest::multipleBenchmarksRowQueries(size_t n, bench_time_t *naiveTime, bench_time_t *queryTime, bench_time_t *cascadingTime, bench_time_t *simpleCascadingTime)
{
    bench_time_t clock1, clock2;
    for (size_t i = 0; i < n; i++) {        
        size_t row = rand() % (_testMatrix->rows());
        
        size_t c1,c2;
        c1 = rand() % (_testMatrix->cols());
        c2 = rand() % (_testMatrix->cols());
        
        Range c = Range(min(c1,c2),max(c1,c2));

        clock1 = now();
        _queryDS->columnTree()->cascadingMaxInRange(row, c);
        clock2 = now();
        
        *cascadingTime = add(*cascadingTime,diff(clock2, clock1));
    }
    for (size_t i = 0; i < n; i++) {

        size_t row = rand() % (_testMatrix->rows());
        
        size_t c1,c2;
        c1 = rand() % (_testMatrix->cols());
        c2 = rand() % (_testMatrix->cols());
        
        Range c = Range(min(c1,c2),max(c1,c2));
        
        clock1 = now();
        _queryDS->columnTree()->simpleCascadingMaxInRange(row, c);
        clock2 = now();
        
        *simpleCascadingTime = add(*simpleCascadingTime,diff(clock2, clock1));
    }
    for (size_t i = 0; i < n; i++) {
        size_t row = rand() % (_testMatrix->rows());
        
        size_t c1,c2;
        c1 = rand() % (_testMatrix->cols());
        c2 = rand() % (_testMatrix->cols());
        
        Range c = Range(min(c1,c2),max(c1,c2));
        
        clock1 = now();
        _queryDS->columnTree()->maxForRowInRange(row, c.min, c.max);
        clock2 = now();
        
        *queryTime = add(*queryTime,diff(clock2, clock1));
    }
    if(SubmatrixQueriesTest::benchmarkNaiveQueries){
        for (size_t i = 0; i < n; i++) {
            size_t row = rand() % (_testMatrix->rows());
            
            size_t c1,c2;
            c1 = rand() % (_testMatrix->cols());
            c2 = rand() % (_testMatrix->cols());
            
            Range c = Range(min(c1,c2),max(c1,c2));

            clock1 = now();
            SubmatrixQueriesTest::naiveMaximumInRow(_testMatrix, c, row);
            clock2 = now();
            
            *naiveTime = add(*naiveTime,diff(clock2, clock1));
        }
    }
}

void SubmatrixQueriesTest::multipleBenchmarksColQueries(size_t n)
{
    bench_time_t naiveTime = ZeroTime, queryTime = ZeroTime, cascadingTime = ZeroTime, simpleCascadingTime = ZeroTime;
    
    multipleBenchmarksColQueries(n,&naiveTime, &queryTime, &cascadingTime,  &simpleCascadingTime);

    cout << "Benchmark for " << n << " row queries:" <<endl;
    cout << "Naive algorithm: " << benchTimeAsMiliSeconds(naiveTime) << " ms" << endl;
    cout << "Canonical nodes: " << benchTimeAsMiliSeconds(queryTime) << " ms" << endl;
    cout << "Cascading: " << benchTimeAsMiliSeconds(cascadingTime) << " ms" << endl;
    cout << "Simple Cascading queries: " << benchTimeAsMiliSeconds(simpleCascadingTime) << " ms" << endl;
    
}

void SubmatrixQueriesTest::multipleBenchmarksColQueries(size_t n, bench_time_t *naiveTime, bench_time_t *queryTime, bench_time_t *cascadingTime, bench_time_t *simpleCascadingTime)
{
    bench_time_t clock1, clock2;
    for (size_t i = 0; i < n; i++) {
        size_t col = rand() % (_testMatrix->cols());
        
        size_t r1,r2;
        r1 = rand() % (_testMatrix->rows());
        r2 = rand() % (_testMatrix->rows());
        
        Range r = Range(min(r1,r2),max(r1,r2));
        
        clock1 = now();
        _queryDS->columnTree()->cascadingMaxInRange(col, r);
        clock2 = now();

        *cascadingTime = add(*cascadingTime,diff(clock2, clock1));
    }
    for (size_t i = 0; i < n; i++) {
        
        size_t col = rand() % (_testMatrix->cols());
        
        size_t r1,r2;
        r1 = rand() % (_testMatrix->rows());
        r2 = rand() % (_testMatrix->rows());
        
        Range r = Range(min(r1,r2),max(r1,r2));
        
        clock1 = now();
        _queryDS->columnTree()->simpleCascadingMaxInRange(col, r);
        
        clock2 = now();

        *simpleCascadingTime = add(*simpleCascadingTime,diff(clock2, clock1));
    }
    for (size_t i = 0; i < n; i++) {
        size_t col = rand() % (_testMatrix->cols());
        
        size_t r1,r2;
        r1 = rand() % (_testMatrix->rows());
        r2 = rand() % (_testMatrix->rows());
        
        Range r = Range(min(r1,r2),max(r1,r2));
        
        clock1 = now();
        _queryDS->columnTree()->maxForRowInRange(col, r.min, r.max);
        clock2 = now();

        *queryTime = add(*queryTime,diff(clock2, clock1));
    }
    if(SubmatrixQueriesTest::benchmarkNaiveQueries){
        for (size_t i = 0; i < n; i++) {
            size_t col = rand() % (_testMatrix->cols());
            
            size_t r1,r2;
            r1 = rand() % (_testMatrix->rows());
            r2 = rand() % (_testMatrix->rows());
            
            Range r = Range(min(r1,r2),max(r1,r2));
            
            clock1 = now();
            SubmatrixQueriesTest::naiveMaximumInRow(_testMatrix, r, col);
            clock2 = now();

            *naiveTime = add(*naiveTime,diff(clock2, clock1));
        }
    }
}


void SubmatrixQueriesTest::multipleBenchmarksSubmatrixQueries(size_t n)
{
    bench_time_t naiveTime = ZeroTime, explicitNodesTime = ZeroTime, implicitNodesTime = ZeroTime;
    
    multipleBenchmarksSubmatrixQueries(n,&naiveTime, &explicitNodesTime, &implicitNodesTime);
    
    cout << "Benchmark for " << n << " submatrix queries:" <<endl;
    cout << "Naive algorithm: " << benchTimeAsMiliSeconds(naiveTime) << " ms" << endl;
    cout << "Explicit canonical nodes: " << benchTimeAsMiliSeconds(explicitNodesTime) << " ms" << endl;
    cout << "Implicit  canonical nodes: " << benchTimeAsMiliSeconds(implicitNodesTime) << " ms" << endl;
    
}

void SubmatrixQueriesTest::multipleBenchmarksSubmatrixQueries(size_t n,bench_time_t *naiveTime, bench_time_t *explicitNodesTime, bench_time_t *implicitNodesTime)
{
    bench_time_t clock1, clock2;
    for (size_t i = 0; i < n; i++) {
        size_t r1, r2;
        r1 = rand() % (_testMatrix->rows());
        r2 = rand() % (_testMatrix->rows());
        
        size_t c1,c2;
        c1 = rand() % (_testMatrix->cols());
        c2 = rand() % (_testMatrix->cols());
        
        
        clock1 = now();
        _queryDS->maxInRange(Range(min(r1,r2),max(r1,r2)),Range(min(c1,c2),max(c1,c2)));
        clock2 = now();
        *explicitNodesTime = add(*explicitNodesTime,diff(clock2, clock1));
    }

    for (size_t i = 0; i < n; i++) {
        size_t r1, r2;
        r1 = rand() % (_testMatrix->rows());
        r2 = rand() % (_testMatrix->rows());
        
        size_t c1,c2;
        c1 = rand() % (_testMatrix->cols());
        c2 = rand() % (_testMatrix->cols());
        
        
        clock1 = now();
        _queryDS->maxInSubmatrix(Range(min(r1,r2),max(r1,r2)),Range(min(c1,c2),max(c1,c2)));
        clock2 = now();
        *implicitNodesTime = add(*implicitNodesTime,diff(clock2, clock1));
    }

    if(SubmatrixQueriesTest::benchmarkNaiveQueries){
        for (size_t i = 0; i < n; i++) {
            size_t r1, r2;
            r1 = rand() % (_testMatrix->rows());
            r2 = rand() % (_testMatrix->rows());
            
            size_t c1,c2;
            c1 = rand() % (_testMatrix->cols());
            c2 = rand() % (_testMatrix->cols());
            
            Range r = Range(min(r1,r2),max(r1,r2));
            Range c = Range(min(c1,c2),max(c1,c2));
            
            clock1 = now();
            SubmatrixQueriesTest::naiveMaximumInSubmatrix(_testMatrix, r, c);
            clock2 = now();
            
            *naiveTime = add(*naiveTime,diff(clock2, clock1));
        }
    }
}

void SubmatrixQueriesTest::multipleBenchmarksAllQueries(size_t nPosition,bench_time_t *positionNaiveTime, bench_time_t *positionExplicitNodesTime, bench_time_t *positionImplicitNodesTime, bench_time_t *positionSimpleCascadingTime,size_t nSubmatrix,bench_time_t *submatrixNaiveTime, bench_time_t *submatrixExplicitNodesTime, bench_time_t *submatrixImplicitNodesTime)
{
    this->multipleBenchmarksSubmatrixQueries(nSubmatrix, submatrixNaiveTime, submatrixExplicitNodesTime,  submatrixImplicitNodesTime);
    this->multipleBenchmarksColQueries(nPosition, positionNaiveTime, positionExplicitNodesTime, positionImplicitNodesTime, positionSimpleCascadingTime);
    this->multipleBenchmarksRowQueries(nPosition, positionNaiveTime, positionExplicitNodesTime, positionImplicitNodesTime, positionSimpleCascadingTime);
}

double SubmatrixQueriesTest::naiveMaximumInColumn(const Matrix<double> *m, Range rowRange, size_t col)
{
    MaxValue<double> max = MaxValue<double>();
    
    for (size_t i = rowRange.min; i <= rowRange.max; i++) {
        max.updateMax((*m)(i,col));
    }
    
    return max.value();
}

double SubmatrixQueriesTest::naiveMaximumInRow(const Matrix<double> *m, Range colRange, size_t row)
{
    MaxValue<double> max = MaxValue<double>();
    
    for (size_t j = colRange.min; j <= colRange.max; j++) {
        max.updateMax((*m)(row,j));
    }
    
    return max.value();
}

double SubmatrixQueriesTest::naiveMaximumInSubmatrix(const Matrix<double> *m, Range rowRange, Range colRange)
{
    MaxValue<double> max = MaxValue<double>();
    
    for (size_t j = colRange.min; j <= colRange.max; j++) {
        max.updateMax(naiveMaximumInColumn(m, rowRange, j));
    }
    
    return max.value();
}

#define LINE_DISTANCE 5

Matrix<double>* SubmatrixQueriesTest::generateInverseMongeMatrixStrip1(size_t rows, size_t cols)
{
    Matrix<double> *m;
    
    cout << "Initializing matrix " << rows << "x" << cols << " ... ";
    try {
        m = new ComplexMatrix<double>(rows,cols);
        cout << "Done" << endl;
    } catch (std::bad_alloc& ba) {
        cout << "\nbad_alloc caught: " << ba.what() << endl;
        cout << "Try to build an other matrix ... ";
        m = new SimpleMatrix<double>(rows,cols);
        cout << "Done" << endl;
    }
    
    cout << "Fill the Inverse Monge Matrix ... " << endl;
    
    int *rowsAbscissa, *colsAbscissa;
    
    rowsAbscissa = new int[rows];
    colsAbscissa = new int[cols];
    
    srand ( time(NULL) );
    
    // we define these values to avoid overflows that will lead to a non inverse Monge matrix
    int max_abscissa = (int)sqrtf(INT_MAX/3) - LINE_DISTANCE;
    int rowInterval = max<int>(max_abscissa/rows,1);
    int colInterval = max<int>(max_abscissa/cols,1);
    
    DEBUG_ASSERT(rowInterval > 0);
    DEBUG_ASSERT(colInterval > 0);
    
    int accumulator =0;
    
    for (size_t i = 0; i < rows; i++) {
        accumulator += rand() % rowInterval;
        rowsAbscissa[i] = accumulator;
    }
    
    accumulator = 0;
    for (size_t j = 0; j < cols; j++) {
        accumulator += rand() % colInterval;
        colsAbscissa[cols-1-j] = accumulator;
    }
    
    for (size_t i = 0; i < rows; i++) {
        for (size_t j = 0; j < cols; j++) {
            double abscissaDiff = rowsAbscissa[i]-colsAbscissa[j];
            
            (*m)(i,j) = sqrt(LINE_DISTANCE + abscissaDiff*abscissaDiff);
        }
    }
    
    delete [] rowsAbscissa;
    delete [] colsAbscissa;
    
    return m;
}

Matrix<double>* SubmatrixQueriesTest::generateInverseMongeMatrixStrip2(size_t rows, size_t cols)
{
    Matrix<double> *m;
    
    if (SubmatrixQueriesTest::verboseBenchmarks) {
        cout << "Initializing matrix " << rows << "x" << cols << " ... ";
    }
    try {
        m = new ComplexMatrix<double>(rows,cols);
        if (SubmatrixQueriesTest::verboseBenchmarks) {
            cout << "Done" << endl;
        }
    } catch (std::bad_alloc& ba) {
        if (SubmatrixQueriesTest::verboseBenchmarks) {
            cout << "\nbad_alloc caught: " << ba.what() << endl;
            cout << "Try to build an other matrix ... ";
        }
        m = new SimpleMatrix<double>(rows,cols);
        if (SubmatrixQueriesTest::verboseBenchmarks) {
            cout << "Done" << endl;
        }
    }
    
    if (SubmatrixQueriesTest::verboseBenchmarks) {
        cout << "Fill the Inverse Monge Matrix ... " << endl;
    }
    
    int *rowsAbscissa, *colsAbscissa;
    
    rowsAbscissa = new int[rows];
    colsAbscissa = new int[cols];
    
    srand ( time(NULL) );
    
    // we define these values to avoid overflows that will lead to a non inverse Monge matrix
    // our goal is to have values in rowsAbscissa and colsAbscissa that are between -max_abscissa and +max_abscissa 
    int max_abscissa;
//    max_abscissa = (int)sqrtf(INT_MAX/3) - LINE_DISTANCE;
    max_abscissa = 0.5*sqrt(INT_MAX-LINE_DISTANCE);
    
    int rowInterval = max<int>(2*max_abscissa/rows,1);
    int colInterval = max<int>(2*max_abscissa/cols,1);
    
    DEBUG_ASSERT(rowInterval > 0);
    DEBUG_ASSERT(colInterval > 0);
    
    int accumulator = -max_abscissa;
    
    for (size_t i = 0; i < rows; i++) {
        accumulator += rand() % rowInterval +1;
        rowsAbscissa[i] = accumulator;
    }
    
    accumulator = -max_abscissa;
    for (size_t j = 0; j < cols; j++) {
        accumulator += rand() % colInterval +1;
        colsAbscissa[cols-1-j] = accumulator;
    }
    
    for (size_t i = 0; i < rows; i++) {
        for (size_t j = 0; j < cols; j++) {
            int abscissaDiff = rowsAbscissa[i]-colsAbscissa[j];
            int diffSquare = abscissaDiff*abscissaDiff;
            int distSquare = LINE_DISTANCE + diffSquare;
            double v = sqrt(distSquare);
            (*m)(i,j) = v;
        }
    }
    
    delete [] rowsAbscissa;
    delete [] colsAbscissa;
    
    return m;
}

Matrix<double>* SubmatrixQueriesTest::generateInverseMongeMatrixSlope(size_t rows, size_t cols)
{
    Matrix<double> *m;
    
    if (SubmatrixQueriesTest::verboseMatrixGeneration) {
        cout << "Initializing matrix " << rows << "x" << cols << " ... ";
    }
    try {
        m = new ComplexMatrix<double>(rows,cols);
        if (SubmatrixQueriesTest::verboseBenchmarks) {
            cout << "Done" << endl;
        }
    } catch (std::bad_alloc& ba) {
        if (SubmatrixQueriesTest::verboseBenchmarks) {
            cout << "\nbad_alloc caught: " << ba.what() << endl;
            cout << "Try to build an other matrix ... ";
        }
        m = new SimpleMatrix<double>(rows,cols);
        if (SubmatrixQueriesTest::verboseBenchmarks) {
            cout << "Done" << endl;
        }
    }
    
    if (SubmatrixQueriesTest::verboseBenchmarks) {
        cout << "Fill the Inverse Monge Matrix ... ";
    }
    bench_time_t t = now();

    vector<double> slope(rows);
    
    srand ( time(NULL) );

    for (size_t i = 0; i < rows; i++) {
        slope[i] = fRand(-0.5*M_PI, 0.5*M_PI);
    }
    std::sort(slope.begin(), slope.end());
    
    for (size_t i = 0; i < rows; i++) {
        for (size_t j = 0; j < cols; j++) {
            double v;
            v = tan(slope[i])*j + rows -1 -i;
            
            (*m)(i,j) = v;
        }
    }
    
    slope = vector<double>(cols);
    for (size_t i = 0; i < cols; i++) {
        slope[i] = fRand(-0.5*M_PI, 0.5*M_PI);
    }
    std::sort(slope.begin(), slope.end());

    for (size_t i = 0; i < cols; i++) {
        for (size_t j = 0; j < rows; j++) {
            double v;
            v = tan(slope[i])*j + rows -1 -i;
            
            (*m)(j,i) += v;
        }
    }

    if (SubmatrixQueriesTest::verboseBenchmarks) {
        t = diff(now(),t);
        cout << " done in " << benchTimeAsMiliSeconds(t) << " ms" << endl;
    }
    
    return m;
}

struct _thread_arg_t
{
    Matrix<double> *matrix;
    vector<double> *slopes;
    size_t min;
    size_t max;
    
    _thread_arg_t(Matrix<double> *m, vector<double> *s, size_t min, size_t max) : matrix(m), slopes(s), min(min), max(max){}
};

void* _fillRows(void *args)
{
    _thread_arg_t input_args = *((_thread_arg_t *)args);
    
    size_t cols = input_args.matrix->cols();
    size_t rows = input_args.matrix->rows();

    for (size_t i = input_args.min; i < input_args.max; i++) {
        for (size_t j = 0; j < cols; j++) {
            double v;
            v = tan((*input_args.slopes)[i])*j + rows -1 -i;
            
            (*input_args.matrix)(i,j) = v;
        }
    }

    
    return NULL;
}

void* _fillCols(void *args)
{
    _thread_arg_t input_args = *((_thread_arg_t *)args);
    
    size_t cols = input_args.matrix->cols();
    size_t rows = input_args.matrix->rows();
    
    for (size_t i = input_args.min; i < input_args.max; i++) {
        for (size_t j = 0; j < rows; j++) {
            double v;
            v = tan((*input_args.slopes)[i])*j + rows -1 -i;
            
            (*input_args.matrix)(j,i) += v;
        }
    }

    return NULL;
}

Matrix<double>* SubmatrixQueriesTest::generateInverseMongeMatrixSlopeMultithread(size_t rows, size_t cols,size_t threadCount)
{
    Matrix<double> *m;
    return m;
}

typedef bench_time_t* clock_ptr;

void SubmatrixQueriesTest::multiBenchmarksPositionQueries(size_t nRows, size_t nCols, size_t nSamples, bench_time_t *naiveTime, bench_time_t *queryTime, bench_time_t *cascadingTime, bench_time_t *simpleCascadingTime)
{
    if (SubmatrixQueriesTest::showProgressBar) cout << "\n";
    for (size_t i = 0; i < nSamples; i++) {
        SubmatrixQueriesTest *t = new SubmatrixQueriesTest(nRows,nCols);
        if (SubmatrixQueriesTest::showProgressBar) cout<< "|" << flush;

        t->multipleBenchmarksColQueries(100, naiveTime, queryTime, cascadingTime, simpleCascadingTime);
        t->multipleBenchmarksRowQueries(100, naiveTime, queryTime, cascadingTime, simpleCascadingTime);
        
        delete t;
    }
}

bench_time_t** SubmatrixQueriesTest::multiSizeBenchmarksPositionQueries(size_t maxNRows, size_t maxNCols, size_t nSampleSize, size_t nSamplePerSize)
{
    bench_time_t **results = new clock_ptr [nSampleSize];
    for (size_t i = 0; i < nSampleSize; i++) {
        results[i] = new bench_time_t [4];
        
#ifdef __MACH__
	results[i][0] = 0;
        results[i][1] = 0;
        results[i][2] = 0;
        results[i][3] = 0;
#else
        results[i][0].tv_sec = 0; results[i][0].tv_nsec = 0;
        results[i][1].tv_sec = 0; results[i][1].tv_nsec = 0;
        results[i][2].tv_sec = 0; results[i][2].tv_nsec = 0;
        results[i][3].tv_sec = 0; results[i][3].tv_nsec = 0;
#endif
        size_t nRows = maxNRows, nCols = maxNCols;
        float fraction = ((float)(i+1))/((float)nSampleSize);
        nRows *= fraction;
        nCols *= fraction;
        
        cout << "Benchmark for size: " << nRows << " x " << nCols << " ... ";

        SubmatrixQueriesTest::multiBenchmarksPositionQueries(nRows,nCols, nSamplePerSize, results[i], results[i]+1, results[i]+2, results[i]+3);
        
        cout << " done\n";
    }
    return results;
}

void SubmatrixQueriesTest::multiBenchmarksSubmatrixQueries(size_t nRows, size_t nCols, size_t nSamples, bench_time_t *naiveTime, bench_time_t *explicitNodesTime, bench_time_t *implicitNodesTime)
{
    if (SubmatrixQueriesTest::showProgressBar) cout << "\n";
    
    for (size_t i = 0; i < nSamples; i++) {
        SubmatrixQueriesTest *t = new SubmatrixQueriesTest(nRows,nCols);
        if (SubmatrixQueriesTest::showProgressBar) cout<< "|" << flush;

        t->multipleBenchmarksSubmatrixQueries(100, naiveTime, explicitNodesTime, implicitNodesTime);
        
        delete t;
    }
}

bench_time_t** SubmatrixQueriesTest::multiSizeBenchmarksSubmatrixQueries(size_t maxNRows, size_t maxNCols, size_t minNRows, size_t minNCols, size_t stepSize, size_t nSamplePerSize, size_t *sampleSize)
{
    size_t size = min((maxNRows - minNRows)/((float)stepSize),(maxNCols - minNCols)/((float)stepSize));
    *sampleSize = size;
    
    bench_time_t **results = new clock_ptr [size];
    size_t nRows = minNRows, nCols = minNCols;
    for (size_t i = 0; nRows <= maxNRows && nCols <= maxNCols; nRows += stepSize, nCols += stepSize, i++) {
        results[i] = new bench_time_t [3];
        
#ifdef __MACH__
        results[i][0] = 0;
        results[i][1] = 0;
        results[i][2] = 0;
#else
        results[i][0].tv_sec = 0; results[i][0].tv_nsec = 0;
        results[i][1].tv_sec = 0; results[i][1].tv_nsec = 0;
        results[i][2].tv_sec = 0; results[i][2].tv_nsec = 0;
#endif

        cout << "Benchmark for size: " << nRows << " x " << nCols << " ... ";
        
        SubmatrixQueriesTest::multiBenchmarksSubmatrixQueries(nRows,nCols, nSamplePerSize, results[i], results[i]+1, results[i]+2);
        
        cout << " done\n";
    }
    return results;
}

void SubmatrixQueriesTest::multiSizeBenchmarksSubmatrixQueries(size_t maxNRows, size_t maxNCols, size_t minNRows, size_t minNCols, size_t stepSize, size_t nSamplePerSize, ostream &outputStream)
{
    size_t nRows = minNRows, nCols = minNCols;
    for (; nRows <= maxNRows && nCols <= maxNCols; nRows += stepSize, nCols += stepSize) {
        bench_time_t *benchmarks = new bench_time_t [3];
        
#ifdef __MACH__
        benchmarks[0] = 0;
        benchmarks[1] = 0;
        benchmarks[2] = 0;
#else
        benchmarks[0].tv_sec = 0; benchmarks[0].tv_nsec = 0;
        benchmarks[1].tv_sec = 0; benchmarks[1].tv_nsec = 0;
        benchmarks[2].tv_sec = 0; benchmarks[2].tv_nsec = 0;
#endif

        cout << "Benchmark for size: " << nRows << " x " << nCols << " ... ";
        
        SubmatrixQueriesTest::multiBenchmarksSubmatrixQueries(nRows,nCols, nSamplePerSize, benchmarks, benchmarks+1, benchmarks+2);
        
        cout << " done\n";
        
        outputStream << ((int)nRows) << " ; " << benchTimeAsMiliSeconds(benchmarks[0]) << " ; " << benchTimeAsMiliSeconds(benchmarks[1]) << " ; " << benchTimeAsMiliSeconds(benchmarks[2]) << "\n";
        outputStream.flush();
        
        delete [] benchmarks;
    }
}

void SubmatrixQueriesTest::multiSizeBenchmarksSubmatrixQueries(size_t maxNRows, size_t maxNCols, size_t nSampleSize, size_t nSamplePerSize, ostream &outputStream)
{
    multiSizeBenchmarksSubmatrixQueries(maxNRows, maxNCols, 0, 0, nSampleSize, nSamplePerSize, outputStream);
}

void SubmatrixQueriesTest::multipleBestPositionAndSubmatrixQueries(size_t nPosQueries, size_t nSMQueries, bench_time_t *positionQueryTime, bench_time_t *submatrixQueryTime)
{    
    bench_time_t clock1, clock2;
    
    for (size_t i = 0; i < nPosQueries; i++) {
        size_t col = rand() % (_testMatrix->cols());
        
        size_t r1, r2;
        r1 = rand() % (_testMatrix->rows());
        r2 = rand() % (_testMatrix->rows());
        
        clock1 = now();
        _queryDS->rowsTree()->simpleCascadingMaxInRange(col, Range(min(r1,r2),max(r1,r2)));
        clock2 = now();
        *positionQueryTime = add(*positionQueryTime,diff(clock2, clock1));
    }
    
    for (size_t i = 0; i < nSMQueries; i++) {
        size_t r1, r2;
        r1 = rand() % (_testMatrix->rows());
        r2 = rand() % (_testMatrix->rows());
        
        size_t c1,c2;
        c1 = rand() % (_testMatrix->cols());
        c2 = rand() % (_testMatrix->cols());
        
        clock1 = now();
        _queryDS->maxInSubmatrix(Range(min(r1,r2),max(r1,r2)),Range(min(c1,c2),max(c1,c2)));
        clock2 = now();
        *submatrixQueryTime = add(*submatrixQueryTime,diff(clock2, clock1));
    }

}

void SubmatrixQueriesTest::multipleBenchmarkBestPositionAndSubmatrixQueries(size_t nRows, size_t nCols, size_t nSamples, size_t nPosQueries, size_t nSMQueries,bench_time_t *positionQueryTime, bench_time_t *submatrixQueryTime)
{
    if (SubmatrixQueriesTest::showProgressBar) cout << "\n";
    
    for (size_t i = 0; i < nSamples; i++) {
        SubmatrixQueriesTest *t = new SubmatrixQueriesTest(nRows,nCols);
        if (SubmatrixQueriesTest::showProgressBar) cout<< "|" << flush;
        
        t->multipleBestPositionAndSubmatrixQueries(nPosQueries, nSMQueries , positionQueryTime, submatrixQueryTime);
        
        delete t;
    }
    
}


void SubmatrixQueriesTest::multiSizeBenchmarkBestPositionAndSubmatrixQueries(size_t maxNRows, size_t maxNCols, size_t minNRows, size_t minNCols, size_t stepSize, size_t nSamplePerSize, size_t nPosQueries, size_t nSMQueries, ostream &outputStream)
{
    size_t nRows = minNRows, nCols = minNCols;
    for (; nRows <= maxNRows && nCols <= maxNCols; nRows += stepSize, nCols += stepSize) {
        bench_time_t *benchmarks = new bench_time_t [2];
        
#ifdef __MACH__
        benchmarks[0] = 0;
        benchmarks[1] = 0;
#else
        benchmarks[0].tv_sec = 0; benchmarks[0].tv_nsec = 0;
        benchmarks[1].tv_sec = 0; benchmarks[1].tv_nsec = 0;
#endif
        
        cout << "Fastest queries benchmarks for size: " << nRows << " x " << nCols << " ...";
        
        SubmatrixQueriesTest::multipleBenchmarkBestPositionAndSubmatrixQueries(nRows,nCols, nSamplePerSize, nPosQueries, nSMQueries, benchmarks, benchmarks+1);
        
        cout << " done\n";
        
        outputStream << ((int)nRows) << " ; " << benchTimeAsMiliSeconds(benchmarks[0]) << " ; " << benchTimeAsMiliSeconds(benchmarks[1]) << "\n";
        outputStream.flush();
        
        delete [] benchmarks;
    }
    
}



void SubmatrixQueriesTest::multiBenchmarksAllQueries(size_t nRows, size_t nCols, size_t nSamples,bench_time_t *positionNaiveTime, bench_time_t *positionExplicitNodesTime, bench_time_t *positionImplicitNodesTime, bench_time_t *positionSimpleCascadingTime,bench_time_t *submatrixNaiveTime, bench_time_t *submatrixExplicitNodesTime, bench_time_t *submatrixImplicitNodesTime)
{
    if (SubmatrixQueriesTest::showProgressBar) cout << "\n";
    
    for (size_t i = 0; i < nSamples; i++) {
        SubmatrixQueriesTest *t = new SubmatrixQueriesTest(nRows,nCols);
        if (SubmatrixQueriesTest::showProgressBar) cout<< "|" << flush;
        
        t->multipleBenchmarksAllQueries(1000, positionNaiveTime, positionExplicitNodesTime, positionImplicitNodesTime, positionSimpleCascadingTime, 100, submatrixNaiveTime, submatrixExplicitNodesTime, submatrixImplicitNodesTime);
        
        delete t;
    }
}

void SubmatrixQueriesTest::multiSizeBenchmarksAllQueries(size_t maxNRows, size_t maxNCols, size_t minNRows, size_t minNCols, size_t stepSize, size_t nSamplePerSize, ostream &outputStream)
{
    size_t nRows = minNRows, nCols = minNCols;
    for (; nRows <= maxNRows && nCols <= maxNCols; nRows += stepSize, nCols += stepSize) {
        bench_time_t *benchmarks = new bench_time_t [7];
        
#ifdef __MACH__
        benchmarks[0] = 0;
        benchmarks[1] = 0;
        benchmarks[2] = 0;
        benchmarks[3] = 0;
        benchmarks[4] = 0;
        benchmarks[5] = 0;
        benchmarks[6] = 0;
#else
        benchmarks[0].tv_sec = 0; benchmarks[0].tv_nsec = 0;
        benchmarks[1].tv_sec = 0; benchmarks[1].tv_nsec = 0;
        benchmarks[2].tv_sec = 0; benchmarks[2].tv_nsec = 0;
        benchmarks[3].tv_sec = 0; benchmarks[3].tv_nsec = 0;
        benchmarks[4].tv_sec = 0; benchmarks[4].tv_nsec = 0;
        benchmarks[5].tv_sec = 0; benchmarks[5].tv_nsec = 0;
        benchmarks[6].tv_sec = 0; benchmarks[6].tv_nsec = 0;
#endif
        
        cout << "Benchmark for size: " << nRows << " x " << nCols << " ... ";
        
        SubmatrixQueriesTest::multiBenchmarksAllQueries(nRows,nCols, nSamplePerSize, benchmarks, benchmarks+1, benchmarks+2, benchmarks+3, benchmarks+4, benchmarks+5, benchmarks+6);
        
        cout << " done\n";
        
        outputStream << ((int)nRows) << " ; " << benchTimeAsMiliSeconds(benchmarks[0]) << " ; " << benchTimeAsMiliSeconds(benchmarks[1]) << " ; " << benchTimeAsMiliSeconds(benchmarks[2]) << " ; " << benchTimeAsMiliSeconds(benchmarks[3]) << " ; " << benchTimeAsMiliSeconds(benchmarks[4]) << " ; " << benchTimeAsMiliSeconds(benchmarks[5]) << " ; " << benchTimeAsMiliSeconds(benchmarks[6]) << "\n";
        outputStream.flush();
        
        delete [] benchmarks;
    }
}

void SubmatrixQueriesTest::multiSizeBenchmarksAllQueries(size_t maxNRows, size_t maxNCols, size_t nSampleSize, size_t nSamplePerSize, ostream &outputStream)
{
    multiSizeBenchmarksAllQueries(maxNRows, maxNCols, 0, 0, nSampleSize, nSamplePerSize, outputStream);
}


void SubmatrixQueriesTest::envelopeSizesForMongeMatrices(size_t nRows, size_t nCols, size_t nSamples, float *rowEnvelopes, float *colEnvelopes, size_t *maxRowSize,size_t *maxColSize)
{
    float totalRowSize = 0., totalColSize = 0.;
    size_t rMax = 0, cMax = 0;
    
    if (SubmatrixQueriesTest::showProgressBar) cout << "\n";
    
    for (size_t i = 0; i < nSamples; i++) {
        
        Matrix<double> *testMatrix;
#if USE_ORACLE_MATRIX
        testMatrix = new OracleMongeMatrix<double>(nRows,nCols);
#elif MULTITHREAD_GENERATION
        testMatrix = generateInverseMongeMatrixSlopeMultithread(nRows, nCols,GENERATION_THREAD_COUNT);
#else
        testMatrix = generateInverseMongeMatrixSlope(nRows, nCols);
#endif
        RowNode<double> *rowTree = new RowNode<double>(testMatrix);
        ColNode<double> *colTree = new ColNode<double>(testMatrix);
        
        if (SubmatrixQueriesTest::showProgressBar) cout<< "|" << flush;
        size_t s;
        
        s = rowTree->maxEnvelopeSize();
        totalRowSize += s;
        rMax = max(rMax,s);
        
        s = colTree->maxEnvelopeSize();
        totalColSize += s;
        cMax = max(cMax,s);

        delete testMatrix;
        delete rowTree;
        delete colTree;
    }
    
    *rowEnvelopes = totalRowSize/((float) nSamples);
    *colEnvelopes = totalColSize/((float) nSamples);
    *maxRowSize = rMax;
    *maxColSize = cMax;
}

void SubmatrixQueriesTest::envelopeSizesStats(size_t maxN, size_t minN, size_t stepSize, bool logSteps, size_t nSamplePerSize, ostream &outputStream)
{
    for (size_t n = minN; n <= maxN;) {
        float rowAvg, colAvg;
        size_t rowMax, colMax;
        
        cout << "Envelopes average size for matrices of size: " << n << " x " << n << " ...";
        SubmatrixQueriesTest::envelopeSizesForMongeMatrices(n, n, nSamplePerSize, &rowAvg, &colAvg, &rowMax, &colMax);
        cout << " done\n";
        
        outputStream << n << " ; " << 0.5*(rowAvg+colAvg) << " ; "<< max(rowMax, colMax)<< endl;
        
        if (logSteps) {
            n*=stepSize;
        }else{
            n+=stepSize;
        }
    }
}

