#include "xboard.h"
#include "boot.h"

#include "c_api_db.h"
#include "c_api_factor.h"

#define FlowUnix(ut) ((ut) >= 12600000 ? ((ut)-5400000) : (ut))
#define FlowRank(ut, freq) ((ut) < 0 ? ((freq) * (-1)) : (ut)) / (freq) + 1

using namespace xtech;

XBoard::XBoard()
{
    basic_db = XDB_Init();
    // x = new double[456];
}

XBoard::~XBoard()
{
    XDB_Cleanup(basic_db);
    for (auto i = _map_pf_t_Cleanup.begin(); i != _map_pf_t_Cleanup.end(); ++i)
    {
        (i->second)(_map_handles[i->first]);
    }
    // delete[] x;
}

void XBoard::InitOnPLH(int *plh)
{
    XDB_InitOnPLH(basic_db, plh);
    // F_InitOnPLH(*handles.begin(), plh);
    for (auto i = _map_pf_t_InitOnPLH.begin(); i != _map_pf_t_InitOnPLH.end(); ++i)
    {
        (i->second)(_map_handles[i->first], plh);
    }
}

void XBoard::InitOnConfig(const char *file_config)
{
    // FILE *p_file_list = fopen(file_config, "r");
    std::ifstream sin(file_config);
    std::string libname;

    while (sin >> libname)
    {
        // printf("DL = %s\n", libname.c_str());
        void *dl_handle = dlopen(libname.c_str(), RTLD_NOW);
        // if (!dl_handle)
        // {
        //     printf("ERROR on dlopen(), %s\n", dlerror());
        // }
        _map_pf_t_Init[libname] = (pf_t_Init)dlsym(dl_handle, "F_Init");
        _map_pf_t_InitOnPLH[libname] = (pf_t_InitOnPLH)dlsym(dl_handle, "F_InitOnPLH");
        _map_pf_t_OnQuote[libname] = (pf_t_OnQuote)dlsym(dl_handle, "F_OnQuote");
        _map_pf_t_GetFactors[libname] = (pf_t_GetFactors)dlsym(dl_handle, "F_GetFactors");
        _map_pf_t_GetFactorsNum[libname] = (pf_t_GetFactorsNum)dlsym(dl_handle, "F_GetFactorsNum");
        _map_pf_t_Cleanup[libname] = (pf_t_Cleanup)dlsym(dl_handle, "F_Cleanup");
        _map_handles[libname] = _map_pf_t_Init[libname]();
    }
}

void XBoard::OnQuote(int *quote)
{
    // F_OnQuote(*handles.begin(), quote, basic_db);
    for (auto i = _map_pf_t_OnQuote.begin(); i != _map_pf_t_OnQuote.end(); ++i)
    {
        (i->second)(_map_handles[i->first], quote, basic_db);
    }
    XDB_OnQuote(basic_db, quote);
    // if (rank_tick < FlowRank(FlowUnix(quote[8]), 3000))
    // {
    //     GetFactors(this->x);
    //     rank_tick = FlowRank(FlowUnix(quote[8]), 3000);
    // }
}

void XBoard::GetFactors(double *factors)
{
    XDB_GetFactors(basic_db, factors);
    // F_GetFactors(*handles.begin(), factors + XDB_GetFactorsNum(basic_db));
    size_t n = XDB_GetFactorsNum(basic_db);
    for (auto i = _map_pf_t_GetFactors.begin(); i != _map_pf_t_GetFactors.end(); ++i)
    {
        // std::cout << i->first << std::endl;
        (i->second)(_map_handles[i->first], factors + n);
        n += _map_pf_t_GetFactorsNum[i->first](_map_handles[i->first]);
    }
}

size_t XBoard::GetFactorsNum()
{
    size_t n = XDB_GetFactorsNum(basic_db);
    for (auto i = _map_pf_t_GetFactorsNum.begin(); i != _map_pf_t_GetFactorsNum.end(); ++i)
    {
        n += (i->second)(_map_handles[i->first]);
    }
    return n;
}