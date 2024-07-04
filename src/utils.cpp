#include "utils.h"

using namespace std;

int64_t get_chrono_ut()
{
    std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch());
    return ms.count();
}

void fields_py_trans(int *fields, int symbol_int)
{
    fields[2] = fields[7] > 0 ? 1 : 2;
    fields[3] = symbol_int;
    if (fields[2] == 1)
    {
        if (fields[5] == 0)
        {
            fields[5] = 1;
        }
        else if (fields[4] == 0)
        {
            fields[4] = fields[5];
            fields[5] = -1;
        }
    }
}

void fields_printf(int *fields)
{
    cout << "[ Fields are: ";
    for (int i = 0; i < 10; i++)
        cout << fields[i] << " ";
    cout << "]" << endl;
}

void _memcpy_df(float *y, double *x, size_t n)
{
    for (size_t i = 0; i < n; ++i)
    {
        y[i] = x[i];
    }
}