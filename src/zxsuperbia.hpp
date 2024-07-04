#ifndef __ZXSUPERBIA__
#define __ZXSUPERBIA__

#include <iostream>

#include <string.h>
#include <math.h>

#include <map>

#define SUPERBIA_DIRECTION_BID 1
#define SUPERBIA_DIRECTION_ASK -1

class ZPointSuperbia
{
private:
public:
    double valueBidOrderVolume;
    double valueAskOrderVolume;
    double valueBidTradeVolume;
    double valueAskTradeVolume;
    double valueBidOrderAmount;
    double valueAskOrderAmount;
    double valueBidTradeAmount;
    double valueAskTradeAmount;
    double valueWeightedBidOrderPrice;
    double valueWeightedAskOrderPrice;
    double valueWeightedBidTradePrice;
    double valueWeightedAskTradePrice;
    // double valueBidOrderVolumeEq;
    // double valueAskOrderVolumeEq;
    // double valueBidTradeVolumeEq;
    // double valueAskTradeVolumeEq;
    // double valueBidOrderAmountEq;
    // double valueAskOrderAmountEq;
    // double valueBidTradeAmountEq;
    // double valueAskTradeAmountEq;
    // double valueCrtOrderLog;
    // double valueCrtTradeLog;
    // double valueCrtOrderImbDif;
    // double valueCrtTradeImbDif;
    // double valueBidPriceEq;
    // double valueAskPriceEq;

    ZPointSuperbia()
    {
        valueBidOrderVolume = 0.0;
        valueAskOrderVolume = 0.0;
        valueBidTradeVolume = 0.0;
        valueAskTradeVolume = 0.0;
        valueBidOrderAmount = 0.0;
        valueAskOrderAmount = 0.0;
        valueBidTradeAmount = 0.0;
        valueAskTradeAmount = 0.0;
        valueWeightedBidOrderPrice = 0.0;
        valueWeightedAskOrderPrice = 0.0;
        valueWeightedBidTradePrice = 0.0;
        valueWeightedAskTradePrice = 0.0;
    }
    ~ZPointSuperbia(){};

    void _AddValueOnOrder(int direction, int price, int volume)
    {
        if (direction == SUPERBIA_DIRECTION_BID)
        {
            valueBidOrderVolume += volume;
            valueBidOrderAmount += 0.01 * volume * price;
        }
        else if (direction == SUPERBIA_DIRECTION_ASK)
        {
            valueAskOrderVolume += volume;
            valueAskOrderAmount += 0.01 * volume * price;
        }
    }
    void _AddValueOnTrade(int direction, int price, int volume)
    {
        if (direction == SUPERBIA_DIRECTION_BID)
        {
            valueBidTradeVolume += volume;
            valueBidTradeAmount += 0.01 * volume * price;
        }
        else if (direction == SUPERBIA_DIRECTION_ASK)
        {
            valueAskTradeVolume += volume;
            valueAskTradeAmount += 0.01 * volume * price;
        }
    }
    void _MakeValueWeightedPrice()
    {
        valueWeightedBidOrderPrice = valueBidOrderAmount / valueBidOrderVolume;
        valueWeightedAskOrderPrice = valueAskOrderAmount / valueAskOrderVolume;
        valueWeightedBidTradePrice = valueBidTradeAmount / valueBidTradeVolume;
        valueWeightedAskTradePrice = valueAskTradeAmount / valueAskTradeVolume;
        // std::cout << "Check! " << valueWeightedAskOrderPrice << std::endl;
    }
    void _Print()
    {
        std::cout << "Here is _Print() of ZPointSuperbia: " << valueBidOrderVolume << " " << valueAskOrderVolume << " " << valueBidTradeVolume << " " << valueAskTradeVolume << " " << valueBidOrderAmount << " " << valueAskOrderAmount << " " << valueBidTradeAmount << " " << valueAskTradeAmount << " " << valueWeightedBidOrderPrice << " " << valueWeightedAskOrderPrice << " " << valueWeightedBidTradePrice << " " << valueWeightedAskTradePrice << std::endl;
    }
};

class ZPointSuperbiaAuxiliary
{
private:
public:
    int rolling_window;
    double valueBidOrderVolumeEq;
    double valueAskOrderVolumeEq;
    double valueBidTradeVolumeEq;
    double valueAskTradeVolumeEq;
    double valueBidOrderAmountNorm;
    double valueAskOrderAmountNorm;
    double valueBidTradeAmountNorm;
    double valueAskTradeAmountNorm;
    double valueBidOrderAmountEq;
    double valueAskOrderAmountEq;
    double valueBidTradeAmountEq;
    double valueAskTradeAmountEq;
    double valueCrtOrderLog;
    double valueCrtTradeLog;
    double valueCrtOrderImbDif;
    double valueCrtTradeImbDif;
    double valueBidPriceEq;
    double valueAskPriceEq;

    ZPointSuperbiaAuxiliary(int _rw)
    {
        rolling_window = _rw;
        valueBidOrderVolumeEq = 0.0;
        valueAskOrderVolumeEq = 0.0;
        valueBidTradeVolumeEq = 0.0;
        valueAskTradeVolumeEq = 0.0;
        valueBidOrderAmountNorm = 0.0;
        valueAskOrderAmountNorm = 0.0;
        valueBidTradeAmountNorm = 0.0;
        valueAskTradeAmountNorm = 0.0;
        valueBidOrderAmountEq = 0.0;
        valueAskOrderAmountEq = 0.0;
        valueBidTradeAmountEq = 0.0;
        valueAskTradeAmountEq = 0.0;
        valueCrtOrderLog = 0.0;
        valueCrtTradeLog = 0.0;
        valueCrtOrderImbDif = 0.0;
        valueCrtTradeImbDif = 0.0;
        valueBidPriceEq = 0.0;
        valueAskPriceEq = 0.0;
    }
    ~ZPointSuperbiaAuxiliary() {}

    void _MakeValue(ZPointSuperbia **_lpoint, double _pb1, double _pa1)
    {
        for (int i = 0; i < rolling_window; ++i)
        {
            valueBidOrderVolumeEq += _lpoint[i]->valueBidOrderAmount;
            valueAskOrderVolumeEq += _lpoint[i]->valueAskOrderAmount;
            valueBidTradeVolumeEq += _lpoint[i]->valueBidTradeAmount;
            valueAskTradeVolumeEq += _lpoint[i]->valueAskTradeAmount;
        }
        valueBidOrderAmountNorm = _lpoint[0]->valueBidOrderAmount / valueBidOrderVolumeEq * rolling_window;
        valueAskOrderAmountNorm = _lpoint[0]->valueAskOrderAmount / valueAskOrderVolumeEq * rolling_window;
        valueBidTradeAmountNorm = _lpoint[0]->valueBidTradeAmount / valueBidTradeVolumeEq * rolling_window;
        valueAskTradeAmountNorm = _lpoint[0]->valueAskTradeAmount / valueAskTradeVolumeEq * rolling_window;

        valueBidOrderVolumeEq /= (rolling_window * _pb1);
        valueAskOrderVolumeEq /= (rolling_window * _pa1);
        valueBidTradeVolumeEq /= (rolling_window * _pb1);
        valueAskTradeVolumeEq /= (rolling_window * _pa1);
    }
    void _MakeValueAmountEq(double *_lobap, double *_lobav, double *_lobbp, double *_lobbv)
    {
        double vlbo = valueBidOrderVolumeEq, vlbt = valueBidTradeVolumeEq, vlao = valueAskOrderVolumeEq, vlat = valueAskTradeVolumeEq;
        for (int i = 0; i < 10; ++i)
        {
            valueBidOrderAmountEq += std::min(_lobav[i], vlbo) * _lobap[i];
            valueBidTradeAmountEq += std::min(_lobav[i], vlbt) * _lobap[i];
            valueAskOrderAmountEq += std::min(_lobbv[i], vlao) * _lobbp[i];
            valueAskTradeAmountEq += std::min(_lobbv[i], vlat) * _lobbp[i];
            vlbo = std::max(0.0, vlbo - _lobav[i]);
            vlbt = std::max(0.0, vlbt - _lobav[i]);
            vlao = std::max(0.0, vlao - _lobbv[i]);
            vlat = std::max(0.0, vlat - _lobbv[i]);
        }
        valueCrtOrderLog = std::log(valueAskOrderAmountEq / valueBidOrderAmountEq);
        valueCrtTradeLog = std::log(valueAskTradeAmountEq / valueBidTradeAmountEq);
        valueCrtOrderImbDif = (valueAskOrderAmountEq - valueBidOrderAmountEq) / (valueAskOrderAmountEq + valueBidOrderAmountEq);
        valueCrtTradeImbDif = (valueAskTradeAmountEq - valueBidTradeAmountEq) / (valueAskTradeAmountEq + valueBidTradeAmountEq);
        valueBidPriceEq = std::log(valueBidTradeAmountEq / valueBidTradeVolumeEq);
        valueAskPriceEq = std::log(valueAskTradeAmountEq / valueAskTradeVolumeEq);
    }
    void _Print()
    {
        std::cout << "Here is _Print() of ZPointSuperbia: "
                  << valueBidOrderVolumeEq
                  << " " << valueAskOrderVolumeEq
                  << " " << valueBidTradeVolumeEq
                  << " " << valueAskTradeVolumeEq
                  << " " << valueBidOrderAmountNorm
                  << " " << valueAskOrderAmountNorm
                  << " " << valueBidTradeAmountNorm
                  << " " << valueAskTradeAmountNorm
                  << " " << valueBidOrderAmountEq
                  << " " << valueAskOrderAmountEq
                  << " " << valueBidTradeAmountEq
                  << " " << valueAskTradeAmountEq
                  << " " << valueCrtOrderLog
                  << " " << valueCrtTradeLog
                  << " " << valueCrtOrderImbDif
                  << " " << valueCrtTradeImbDif
                  << " " << valueBidPriceEq
                  << " " << valueAskPriceEq
                  << std::endl;
    }
};

class ZPointSuperbiaFinal
{
public:
    ZPointSuperbiaAuxiliary *aux1, *aux5;
    ZPointSuperbia *data;
    ZPointSuperbiaFinal()
    {
        aux1 = new ZPointSuperbiaAuxiliary(1);
        aux5 = new ZPointSuperbiaAuxiliary(5);
        data = new ZPointSuperbia();
    }
    ~ZPointSuperbiaFinal() {}
};

class ZBufferSuperbia
{
private:
    /* data */

public:
    int _max_timehouse;
    int last_time;
    std::map<int, ZPointSuperbiaFinal> point_timehouse;

    ZPointSuperbia **_buf_superbia;
    double *_buf_lob10;

    void NewTimehouse(int t)
    {
        point_timehouse[t] = ZPointSuperbiaFinal();
        last_time = t;
    }
    void DelTimehouse(int t)
    {
        point_timehouse.erase(t);
    }
    void FixTimehouse()
    {
        int _size = point_timehouse.size();
        if (_size <= _max_timehouse)
            return;
        auto ib = point_timehouse.begin(), ie = point_timehouse.end();
        for (auto i = ib; i != ie; ++i)
        {
            point_timehouse.erase(i);
            _size--;
            if (_size <= _max_timehouse)
                return;
        }
    }

    ZBufferSuperbia(int mth)
    {
        _max_timehouse = mth;
        last_time = 0;
        _buf_superbia = new ZPointSuperbia *[5];
        _buf_lob10 = new double[40];
    }
    ~ZBufferSuperbia()
    {
        delete[] _buf_superbia;
        delete[] _buf_lob10;
    }
};

// ZBufferSuperbia::ZBufferSuperbia(int mth)
// {
//     _max_timehouse = mth;
//     last_time = 0;
//     _buf_superbia = new ZPointSuperbia *[5];
//     _buf_lob10 = new double[40];
// }

// ZBufferSuperbia::~ZBufferSuperbia()
// {
//     delete[] _buf_superbia;
//     delete[] _buf_lob10;
// }

// void ZBufferSuperbia::NewTimehouse(int t)
// {
//     point_timehouse[t] = ZPointSuperbiaFinal();
//     last_time = t;
// }

// void ZBufferSuperbia::DelTimehouse(int t)
// {
//     point_timehouse.erase(t);
// }

// void ZBufferSuperbia::FixTimehouse()
// {
//     int _size = point_timehouse.size();
//     if (_size <= _max_timehouse)
//         return;
//     auto ib = point_timehouse.begin(), ie = point_timehouse.end();
//     for (auto i = ib; i != ie; ++i)
//     {
//         point_timehouse.erase(i);
//         _size--;
//         if (_size <= _max_timehouse)
//             return;
//     }
// }

class ZBufferSuperbiaLV6
{
private:
    /* data */
public:
    int last_time;
    int time_offset;
    double *price;
    double *volume;
    double *ans;
    int d_offset;
    double _tmpp;

    ZBufferSuperbiaLV6()
    {
        last_time = 0;
        time_offset = 0;
        d_offset = 0;
        price = new double[10];
        volume = new double[40];
        ans = new double[4];
        memset(price, 0, sizeof(double) * 10);
        memset(volume, 0, sizeof(double) * 40);
        memset(ans, 0, sizeof(double) * 4);
    };
    ~ZBufferSuperbiaLV6()
    {
        delete[] price;
        delete[] volume;
        delete[] ans;
    };

    void _OnNewTime(int t)
    {
        last_time = t;
        time_offset = t % 10;
        for (int i = 0; i < 4; ++i)
        {
            ans[i] += volume[10 * i + (time_offset + 9) % 10];
            ans[i] -= volume[10 * i + time_offset];
            volume[10 * i + time_offset] = 0.0;
        }
        // std::cout << "Check print on new time: " << t << " " << ans[0] << " " << ans[1] << " " << ans[2] << " " << ans[3] << std::endl;
    }

    void _Modify(int d, int p, int v)
    {
        _tmpp = price[(time_offset + 9) % 10];
        if (_tmpp > 0)
        {
            d_offset = d > 0 ? (time_offset) : (20 + time_offset);
            if (p > _tmpp)
            {
                volume[d_offset] += 0.01 * p * v;
                if (p > _tmpp * (1.0 + 0.01 * d))
                {
                    volume[10 + d_offset] += 0.01 * p * v;
                }
            }
        }
        price[(time_offset + 9) % 10] = p;
    }
};

#endif
