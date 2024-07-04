#ifndef __ZSUPGENUS__
#define __ZSUPGENUS__

#include <iostream>
#include <fstream>

#include "../Tools/zspmat.h"

namespace lits
{
    class ZPointGenusOfflineData
    {
    private:
        /* data */
    public:
        ZPointGenusOfflineData(int, int, char const *, char const *);
        ~ZPointGenusOfflineData();

        int _cols, _rows;
        int *_snapshot_lob_last;
        int *_snapshot_lob_this;
        int *_snapshot_ofl_last;
        lits::ZPointSpMatCSR _lob, _ofl;
        std::ofstream _of_lob, _of_ofl;

        void WriteFile();
        // void RefreshSnapshotLOB();
        void RefreshSnapshotOFL();
    };
}

#endif