#ifndef __FACTOR__
#define __FACTOR__

#include <iostream>
#include <cmath>

// #include "OpenBLAS/cblas.h"
#include <cblas.h>

constexpr int rolling_windows[] = {0, 4, 8, 16, 32, 64};
constexpr int n_rolling_windows = 6;
constexpr int num_windows = 8;
constexpr int params_macd[] = {12, 26, 9};

int zblas_imax(int a, int b);
int zblas_imin(int a, int b);
double zblas_dmax(double a, double b);
double zblas_dmin(double a, double b);
double zblas_dabs(double a);
double zblas_dvwi(const int n, const double *x, const double offset);

int factor_IdxPrice(double *_x, int _time, int LOB, double *_idx_price);

int factor_MACDs(double nx, double *m0, double *m1, const int *par);

#endif