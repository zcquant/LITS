#ifndef __DATAPOINT__
#define __DATAPOINT__

#include <iostream>

#include <map>

#include <Xtech/config.h>

template <class T>
class Zqueue
{
public:
    Zqueue();
    ~Zqueue();
    int max_size;
    int n;
    T **buf;
    inline void _new_all(int msz);
    inline int size();
    inline void push(T *x);
    inline T *front();
    inline T *back();
};

template <class T>
Zqueue<T>::Zqueue()
{
    buf = NULL;
}

template <class T>
Zqueue<T>::~Zqueue()
{
    for (int i = 0; i < max_size; i++)
    {
        delete buf[i];
    }
    if (sizeof(buf) > 0)
        delete[] buf;
}

template <class T>
inline void Zqueue<T>::_new_all(int msz)
{
    max_size = msz;
    n = 0;
    buf = new T *[msz];
}

template <class T>
inline int Zqueue<T>::size()
{
    return n;
}

template <class T>
inline void Zqueue<T>::push(T *x)
{
    if (max_size == 0)  max_size = 64;
    if (n >= max_size)
        delete buf[n % max_size];
    *(buf + (n % max_size)) = x;
    n++;
}

template <class T>
inline T *Zqueue<T>::front()
{
    if (max_size == 0)  max_size = 64;
    if (n < max_size)
        return *buf;
    return *(buf + (n % max_size));
}

template <class T>
inline T *Zqueue<T>::back()
{
    if (max_size == 0)  max_size = 64;
    // std::cout << "Check max_size!" << max_size << std::endl;
    return *(buf + ((n - 1) % max_size));
}

template <class T>
class ZTS
{
private:
public:
    int _max_ts;
    std::map<int, T *> _history;
    ZTS(int mts);
    ~ZTS();
    // void NewPoint(int t);
    void NewPoint(int t, int _t_init_argv);
    void DelPoint(int t);
    void FixPoint();
};

template <class T>
ZTS<T>::ZTS(int mts)
{
    _max_ts = mts;
}

template <class T>
ZTS<T>::~ZTS()
{
}

// template <class T>
// void ZTS<T>::NewPoint(int t)
// {
//     _history[t] = T();
// }

template <class T>
void ZTS<T>::NewPoint(int t, int _t_init_argv)
{
    // _history[t] = T();
    _history[t] = new T(_t_init_argv);
}

template <class T>
void ZTS<T>::DelPoint(int t)
{
    delete _history[t];
    _history.erase(t);
}

template <class T>
void ZTS<T>::FixPoint()
{
    int _size = _history.size();
    if (_size <= _max_ts)
        return;
    auto ib = _history.begin(), ie = _history.end();
    for (auto i = ib; i != ie; ++i)
    {
        delete _history[i];
        _history.erase(i);
        --_size;
        if (_size <= _max_ts)
            return;
    }
}

class L2DataPointStructed
{
public:
    L2DataPointStructed();
    ~L2DataPointStructed();
    int rank_tick, ut;
    int len_lob;
    int *orderbook, *orderflow_bid, *orderflow_ask, *orderflow_vol;
    void _new_all(int llob);

private:
};

class L2DataPointUnstructed
{
public:
    L2DataPointUnstructed();
    ~L2DataPointUnstructed();
    int rank_tick, ut;
    int idx_bid_1, idx_ask_1;
    double idx_mid;
    double pwv_bid;
    double pwv_ask;
    double pwv_imb;
    double delta;
    double delta_acc;
    double volume;

private:
};

class L2DataPointMACDs
{
public:
    L2DataPointMACDs();
    ~L2DataPointMACDs();
    int rank_tick, ut;
    double *macd_idxprice;
    double *macd_pwv_bid;
    double *macd_pwv_ask;
    double *macd_pwv_imb;
    void _new_all();

private:
};

// class ZPTDiceTemplate
// {
// private:
// public:
//     ZPTDiceTemplate() {}
//     ~ZPTDiceTemplate() {}
//     int diceSides;
//     double *diceValues;
//     // virtual void ModifyDiceValue() = 0;
// };

// class ZPTDiceNaiveSix : public ZPTDiceTemplate
// {
// private:
//     /**
//      * diceValues
//      * 6-dim
//      * bidLimit, bidMarket, bidCancel, askLimit, askMarket, askCancel
//      */
// public:
//     // int diceSides;
//     // double *diceValues;
//     ZPTDiceNaiveSix(int sides);
//     ~ZPTDiceNaiveSix();
//     void ModifyDiceValue(int flag, int symbol, int _id0, int _id1, int v);
//     void MakeRightValue();
//     void GetValue(double *_x);
// };

// ZPTDiceNaiveSix::ZPTDiceNaiveSix(int sides)
// {
//     diceSides = sides;
//     diceValues = new double[diceSides];
// }

// ZPTDiceNaiveSix::~ZPTDiceNaiveSix()
// {
//     delete[] diceValues;
// }

// void ZPTDiceNaiveSix::ModifyDiceValue(int flag, int symbol, int _id0, int _id1, int v)
// {
//     if (flag == 1)
//     {
//         if (_id1 > 0)
//         {
//             diceValues[0] += v;
//         }
//         else if (_id1 < 0)
//         {
//             diceValues[3] += v;
//         }
//     }
//     else if (flag == 2)
//     {
//         if (_id1 == 0)
//         {
//             diceValues[2] -= v;
//         }
//         else if (_id0 == 0)
//         {
//             diceValues[5] -= v;
//         }
//         else if (_id0 > _id1)
//         {
//             diceValues[1] -= v;
//             if (symbol >= 600000)
//             {
//                 diceValues[0] -= v;
//             }
//         }
//         else if (_id0 < _id1 || symbol >= 600000)
//         {
//             diceValues[4] -= v;
//             if (symbol >= 600000)
//             {
//                 diceValues[3] -= v;
//             }
//         }
//     }
// }

// void ZPTDiceNaiveSix::MakeRightValue()
// {
//     diceValues[0] -= diceValues[1];
//     diceValues[3] -= diceValues[4];
// }

// void ZPTDiceNaiveSix::GetValue(double *_x)
// {
//     _x[0] = diceValues[0];
//     _x[1] = diceValues[1];
//     _x[2] = diceValues[2];
//     _x[3] = diceValues[3];
//     _x[4] = diceValues[4];
//     _x[5] = diceValues[5];
// }

#endif
