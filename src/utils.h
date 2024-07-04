#ifndef __UTILS__
#define __UTILS__

#include <iostream>

#include <string>

#include <chrono>

#include <stdint.h>

#define FlowUnix(ut) ((ut) >= 12600000 ? ((ut)-5400000) : (ut))
#define FlowRank(ut, freq) ((ut) < 0 ? ((freq) * (-1)) : (ut)) / (freq) + 1

class Filenames
{
public:
    char file_bin_flow[128], file_bin_factors[128], file_bin_lob[128], file_bin_ofl[128];
};

int64_t get_chrono_ut();

void fields_py_trans(int *fields, int symbol_int);

void fields_printf(int *fields);

void _memcpy_df(float *y, double *x, size_t n);

#endif