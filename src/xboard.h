#ifndef __FF__XBOARD__
#define __FF__XBOARD__

#include <iostream>
#include <fstream>
#include <map>
#include <set>
#include <string>

#include <stddef.h>
#include <dlfcn.h>

#include "c_api_db.h"

typedef void *(*pf_t_Init)();
typedef void (*pf_t_InitOnPLH)(void *, int *);
typedef void (*pf_t_OnQuote)(void *, int *, void *);
typedef void (*pf_t_GetFactors)(void *, double *);
typedef size_t (*pf_t_GetFactorsNum)(void *);
typedef void (*pf_t_Cleanup)(void *);

// typedef void ()

// void *F_Init();
// void F_InitOnPLH(void *, int *);
// void F_OnQuote(void *, int *, void *);
// void F_GetFactors(void *, double *);
// size_t F_GetFactorsNum(void *);
// void F_Cleanup(void *);

namespace xtech
{
    class XBoard
    {
    private:
        // int rank_tick = 0;
        void *basic_db = nullptr;
        // std::set<void *> handles;
        std::map<std::string, void *> _map_handles{};
        std::map<std::string, pf_t_Init> _map_pf_t_Init{};
        std::map<std::string, pf_t_InitOnPLH> _map_pf_t_InitOnPLH{};
        std::map<std::string, pf_t_OnQuote> _map_pf_t_OnQuote{};
        std::map<std::string, pf_t_GetFactors> _map_pf_t_GetFactors{};
        std::map<std::string, pf_t_GetFactorsNum> _map_pf_t_GetFactorsNum{};
        std::map<std::string, pf_t_Cleanup> _map_pf_t_Cleanup{};

    public:
        // double *x;
        XBoard();
        ~XBoard();
        void InitOnPLH(int *);

        void InitOnConfig(const char *);
        void OnQuote(int *);
        void GetFactors(double *);
        size_t GetFactorsNum();

        // void GetLOB()
        // {
        //     const char* fileName = "lob.bin";
        //     std::ofstream outFile(fileName, std::ios::binary | std::ios::app);
        //     int lob[327];
        //     XDB_GetLOB(basic_db, lob);
        //     // for (int i = 0; i < 327; ++i)
        //     //     std::cout << lob[i] << " ";
        //     // std::cout << std::endl;
        //     outFile.write(reinterpret_cast<const char*>(&lob[0]), 327 * sizeof(int));
        //     outFile.close();
        // }
    };

} // namespace xtech

#endif