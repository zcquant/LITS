#ifdef __cplusplus
extern "C"
{
#endif

#ifndef __CAPI__FACTOR__
#define __CAPI__FACTOR__

#include "stddef.h"

    void *F_Init();                         // return handle of factor
    void F_InitOnPLH(void *, int *);        // input: handle and 8-field PLH
    void F_OnQuote(void *, int *, void *);  // input: handle, 10-field quote and Xdb
    void F_GetFactors(void *, double *);    // input: handle and memory for factors
    size_t F_GetFactorsNum(void *);         // input: handle
    void F_Cleanup(void *);                 // free memory

#endif

#ifdef __cplusplus
}
#endif