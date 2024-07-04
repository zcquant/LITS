#ifdef __cplusplus
extern "C"
{
#endif

#ifndef __CAPI__DB__
#define __CAPI__DB__

#include "stddef.h"

    void XDB_GetLOB(void *Xdb, int *lob);
    void XDB_GetOFL(void *Xdb, int *ofl);
    void XDB_GetVolume(void *Xdb, double *volume);
    void XDB_GetAmount(void *Xdb, double *amount);
    void XDB_QUOTE_XH(int *quote_X, int *quote_H);  // quote_X: 10-field input; quote_H: 5-field output

#endif

#ifdef __cplusplus
}
#endif