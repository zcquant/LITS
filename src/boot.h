#ifndef __FF__DASHBOARD__
#define __FF__DASHBOARD__

#include <stddef.h>

void* XDB_Init();
void XDB_InitOnPLH(void *, int *);
void XDB_OnQuote(void *, int *);
void XDB_GetFactors(void *, double *);
size_t XDB_GetFactorsNum(void *);
void XDB_Cleanup(void *);

#endif