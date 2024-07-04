#ifndef __ZSPMAT__
#define __ZSPMAT__

#include <iostream>

#include <vector>

#include <string.h>

namespace lits
{
    class ZPointSpMatCSR
    {
    private:
        /* data */
    public:
        ZPointSpMatCSR(int);
        ~ZPointSpMatCSR();

        int _rows;
        int *_row_offset;
        std::vector<int> _col_indices, _values;

        void MakeRowOffset(int);
        void PushBack(int, int);
        void CompRowOffsets();
    };
}

#endif