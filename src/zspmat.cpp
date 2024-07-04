#include "zspmat.h"

lits::ZPointSpMatCSR::ZPointSpMatCSR(int Rows)
{
    _rows = Rows;
    _row_offset = new int[Rows + 1];
    memset(_row_offset, 0, sizeof(int) * (Rows + 1));
}

lits::ZPointSpMatCSR::~ZPointSpMatCSR()
{
    delete[] _row_offset;
}

void lits::ZPointSpMatCSR::PushBack(int Col, int Val)
{
    _col_indices.push_back(Col);
    _values.push_back(Val);
}

void lits::ZPointSpMatCSR::MakeRowOffset(int Row)
{
    _row_offset[Row] = _values.size();
}

void lits::ZPointSpMatCSR::CompRowOffsets()
{
    for (int i = _rows; i >= 1; i--)
    {
        if (_row_offset[i - 1] > _row_offset[i])
        {
            for (int j = i; j < _rows + 1; j++)
            {
                _row_offset[j] = _row_offset[i - 1];
            }
            break;
        }
    }
}