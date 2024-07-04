#include "c_api_db.h"

#include "boot.h"

#include "dashboard.h"

void *XDB_Init()
{
    return new Dashboard;
}
void XDB_InitOnPLH(void *Xdb, int *plh)
{
    // std::cout << "XDB_InitOnPLH(void *Xdb, int *plh)" << std::endl;
    ((Dashboard *)Xdb)->set_info(plh);
    ((Dashboard *)Xdb)->_new_all();
}
void XDB_OnQuote(void *Xdb, int *quote)
{
    ((Dashboard *)Xdb)->on_l2fields(quote);
}
void XDB_GetFactors(void *Xdb, double *factors)
{
    for (int i = 0; i < ((Dashboard *)Xdb)->p_features; ++i)
        factors[i] = ((Dashboard *)Xdb)->features[i];
}
size_t XDB_GetFactorsNum(void *Xdb)
{
    return ((Dashboard *)Xdb)->p_features;
}
void XDB_GetLOB(void *Xdb, int *lob)
{
    ((Dashboard *)Xdb)->_Snapshot_LOB(lob);
}
void XDB_GetOFL(void *Xdb, int *ofl)
{
    for (int i = 0; i < ((Dashboard *)Xdb)->len_lob; ++i)
        ofl[i] = ((Dashboard *)Xdb)->OFL[i];
}
void XDB_GetVolume(void *Xdb, double *volume)
{
    *volume = ((Dashboard *)Xdb)->Volume;
}
void XDB_GetAmount(void *Xdb, double *amount)
{
    *amount = ((Dashboard *)Xdb)->Amount;
}
void XDB_Cleanup(void *Xdb)
{
    delete ((Dashboard *)Xdb);
}

void XDB_QUOTE_XH(int *quote_X, int *quote_H)
{
    int flag = quote_X[2], _id0 = quote_X[4], _id1 = quote_X[5], p = quote_X[6], v = quote_X[7], ut = quote_X[8];
    switch (flag)
    {
    case 1:
        if (_id1 == 1)
        {
            quote_H[0] = _id0;
            quote_H[1] = 0;
            quote_H[2] = p;
            quote_H[3] = v;
            quote_H[4] = ut;
        }
        else
        {
            quote_H[0] = 0;
            quote_H[1] = _id0;
            quote_H[2] = p;
            quote_H[3] = v;
            quote_H[4] = ut;
        }
        break;
    case 2:
        quote_H[0] = _id0;
        quote_H[1] = _id1;
        quote_H[2] = p;
        quote_H[3] = v;
        quote_H[4] = ut;
        break;
    default:
        break;
    }
    return;
}