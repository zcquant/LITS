#include "zsupgenus.h"

lits::ZPointGenusOfflineData::ZPointGenusOfflineData(int Cols, int Rows, char const *OF_LOB, char const *OF_OFL) : _lob(Rows), _ofl(Rows)
{
    _cols = Cols;
    _rows = Rows;
    _snapshot_lob_last = new int[Cols];
    _snapshot_lob_this = new int[Cols];
    _snapshot_ofl_last = new int[Cols];
    memset(_snapshot_lob_last, 0, sizeof(int) * Cols);
    memset(_snapshot_lob_this, 0, sizeof(int) * Cols);
    memset(_snapshot_ofl_last, 0, sizeof(int) * Cols);
    _of_lob = std::ofstream(OF_LOB, std::ios::out | std::ios::binary);
    _of_ofl = std::ofstream(OF_OFL, std::ios::out | std::ios::binary);
}

lits::ZPointGenusOfflineData::~ZPointGenusOfflineData()
{
    // cout << "Using ~ZPointGenusOfflineData() here!" << endl;
    delete[] _snapshot_lob_last;
    delete[] _snapshot_lob_this;
    delete[] _snapshot_ofl_last;
    _of_lob.close();
    _of_ofl.close();
}

void lits::ZPointGenusOfflineData::WriteFile()
{
    _lob.CompRowOffsets();
    _ofl.CompRowOffsets();
    // cout << "Working: WriteFile of ZPointGenusOfflineData" << endl;
    int num = _lob._row_offset[_rows];
    _of_lob.write((char *)_lob._row_offset, sizeof(int) * (_rows + 1));
    _of_lob.write((char *)(&(_lob._col_indices[0])), sizeof(int) * num);
    _of_lob.write((char *)(&(_lob._values[0])), sizeof(int) * num);
    num = _ofl._row_offset[_rows];
    _of_ofl.write((char *)_ofl._row_offset, sizeof(int) * (_rows + 1));
    _of_ofl.write((char *)(&(_ofl._col_indices[0])), sizeof(int) * num);
    _of_ofl.write((char *)(&(_ofl._values[0])), sizeof(int) * num);
}

void lits::ZPointGenusOfflineData::RefreshSnapshotOFL()
{
    memset(_snapshot_ofl_last, 0, _cols);
}