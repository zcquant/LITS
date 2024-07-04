#ifndef __ZXBASIC__
#define __ZXBASIC__

#include <stdio.h>
#include <string.h>
#include <math.h>

/*

columns1 = [
    'PreClosePrice',        # 昨收价
    'OpenPrice',            # 开盘价 / 昨收价
    'HighPrice',            # 最高价 / 昨收价
    'LowPrice',             # 最低价 / 昨收价
    'LastPrice',            # 最新价
    'TotalNo',              # 总成交笔数
    'TotalVolume',          # 总成交量
    'TotalAmount',          # 总成交额
    'TotalSellOrderVolume', # 总卖方委托量
    'WtAvgSellPrice',       # 加权平均卖价
    'SellPrice02',          # AP2
    'SellPrice01',          # AP1
    'SellVolume02',         # AV2
    'SellVolume01',         # AV1
    'WtAvgBuyPrice',        # 加权平均买价
    'BuyPrice01',           # BP1
    'BuyPrice02',           # BP2
    'BuyVolume01',          # BV1
    'BuyVolume02',          # BV2
    'TotalBuyOrderNo01',    # 总买方委托笔数1
    'TotalBuyOrderNo02',    # 总买方委托笔数2
    'TotalSellOrderNo01',   # 总卖方委托笔数1
    'TotalSellOrderNo02',   # 总卖方委托笔数2
    'TradeAmount',          # 3s成交额
    'TradeNo',              # 3s成交笔数
    'TradeVolume',          # 3s成交量
    'SumAskSize',           # Ask10档总量
    'MaxAskSize',           # Ask10档最大量
    'SumBidSize',           # Bid10档总量
    'MaxBidSize',           # Bid10档最大量
    'MaxAskSizePrice',      # Ask10档最大量位置价格
    'MaxBidSizePrice',      # Bid10档最大量位置价格
    'SellMoney01',          # AA1
    'SellMoney02',          # AA2
    'BuyMoney01',           # BA1
    'BuyMoney02',           # BA2
    'SumAskMoney',          # SUM(AA)
    'SumBidMoney',          # SUM(BA)
    'AverageAskPrice',      # 平均卖价
    'AverageBidPrice',      # 平均买价
    'money',                # 3s成交额 / (SUM(AA) + SUM(BA))
    'rank',
    'ma',                   # 总成交额 / 总成交量 / 昨收价
    'pld',                  # 最新价 / 昨收价 - MA
    'zf',                   # 最高价 - 最低价
    'hzf',                  # 最高价 - 开盘价
    'lzf',                  # 最低价 - 开盘价
    'spread1',              # (AP1 - BP1) / 昨收价
    'spread2',              # (平均卖价 - 平均买价) / 昨收价
    'spread3',              # (最大量卖价 - 最大量买价) / 昨收价
    'ma5',                  # MA(最新价, 5)
    'ma15',                 # MA(最新价, 15)
    'std',                  # MA5 20窗口标准差
    'std1',                 # 最新价 5窗口标准差
    'unit',                 # 1 / 昨收价
    'pld_std',              # pld 5窗口标准差
    'return1',              # 最新价相较过去1窗口变化率
    'return2',
    'return3',
    'return4',
]

*/

namespace lits
{
    class ZHelperXBasic
    {
    public:
        double bv, av, ba, aa;
        size_t imbv, imav;
        void RefreshHelper()
        {
            bv = 0;
            av = 0;
            ba = 0;
            aa = 0;
            imbv = 0;
            imav = 0;
        }
    };

    class ZBoardXBasic
    {
    private:
        ZHelperXBasic h;
        double _x[60];
        double pre_close_price, open_price, high_price, low_price, total_no, total_volume, total_amount, trade_no, trade_volume, trade_amount;
        double sum_bid_volume, sum_bid_amount, sum_ask_volume, sum_ask_amount;
        size_t rank = 0;
        double last_price[15], pld[5], ma5[20];

    public:
        double _lob[40];
        double total_bid_order_1, total_bid_order_2, total_ask_order_1, total_ask_order_2;
        ZBoardXBasic()
        {
            memset(_x, 0, sizeof(double) * 60);
            memset(last_price, 0, sizeof(double) * 15);
            memset(pld, 0, sizeof(double) * 5);
            memset(ma5, 0, sizeof(double) * 20);
            memset(_lob, 0, sizeof(double) * 40);
            open_price = -1;
            high_price = -1;
            low_price = -1;
            total_no = 0;
            total_volume = 0;
            total_amount = 0;
            trade_no = 0;
            trade_volume = 0;
            trade_amount = 0;
            sum_bid_volume = 0;
            sum_bid_amount = 0;
            sum_ask_volume = 0;
            sum_ask_amount = 0;
        };
        ~ZBoardXBasic(){};
        void Init(double _pre_close_price)
        {
            pre_close_price = _pre_close_price;
            _x[0] = _pre_close_price;
            _x[54] = 1.0 / _pre_close_price;
        }
        void RefreshLOB()
        {
            for (int i = 0; i < 40; ++i)
            {
                _lob[i] = nan("");
            }
            total_bid_order_1 = 0;
            total_bid_order_2 = 0;
            total_ask_order_1 = 0;
            total_ask_order_2 = 0;
        }
        void ModifyOnTrade(double _price, double _volume)
        {
            if (open_price == -1)
            {
                open_price = _price;
                _x[1] = _price / pre_close_price;
            }
            if (high_price == -1 || high_price < _price)
            {
                high_price = _price;
            }
            if (low_price == -1 || low_price > _price)
            {
                low_price = _price;
            }
            last_price[rank % 15] = _price;
            // printf("Modify last price of rank = %lu to %.2lf\n", rank, _price);
            ++total_no;
            total_volume += _volume;
            total_amount += _price * _volume;
            ++trade_no;
            trade_volume += _volume;
            trade_amount += _price * _volume;
        }
        void ModifyOnOrder(double _price, double _volume, int _d)
        {
            if (_d == 1)
            {
                sum_bid_volume += _volume;
                sum_bid_amount += _price * _volume;
            }
            else if (_d == -1)
            {
                sum_ask_volume += _volume;
                sum_ask_amount += _price * _volume;
            }
        }
        void CalcX(int _rank)
        {
            h.RefreshHelper();
            for (size_t i = 0; i < 10; ++i)
            {
                h.bv += _lob[10 + i];
                h.av += _lob[30 + i];
                h.ba += _lob[i] * _lob[10 + i];
                h.aa += _lob[20 + i] * _lob[30 + i];
                if (_lob[10 + i] > _lob[10 + h.imbv])
                    h.imbv = i;
                if (_lob[30 + i] > _lob[30 + h.imav])
                    h.imav = i;
            }
            rank = _rank;
            _x[2] = high_price / pre_close_price;
            _x[3] = low_price / pre_close_price;
            _x[4] = last_price[rank % 15];
            _x[5] = total_no;
            _x[6] = total_volume;
            _x[7] = total_amount;
            _x[8] = sum_ask_volume;
            _x[9] = sum_bid_amount / sum_bid_volume;
            _x[10] = _lob[21] / pre_close_price;
            _x[11] = _lob[20] / pre_close_price;
            _x[12] = _lob[31] * 20.0 / (h.bv + h.av);
            _x[13] = _lob[30] * 20.0 / (h.bv + h.av);
            _x[14] = sum_ask_amount / sum_ask_volume;
            _x[15] = _lob[0] / pre_close_price;
            _x[16] = _lob[1] / pre_close_price;
            _x[17] = _lob[10] * 20.0 / (h.bv + h.av);
            _x[18] = _lob[11] * 20.0 / (h.bv + h.av);
            _x[19] = total_bid_order_1;
            _x[20] = total_bid_order_2;
            _x[21] = total_ask_order_1;
            _x[22] = total_ask_order_2;
            _x[23] = trade_amount;
            _x[24] = trade_no;
            _x[25] = trade_volume;
            _x[26] = h.av;
            _x[27] = _lob[30 + h.imav] * 20.0 / (h.av + h.bv);
            _x[28] = h.bv;
            _x[29] = _lob[10 + h.imbv] * 20.0 / (h.av + h.bv);
            _x[30] = _lob[20 + h.imav] / pre_close_price;;
            _x[31] = _lob[h.imbv] / pre_close_price;;
            _x[32] = _lob[20] * _lob[30] * 20.0 / (h.aa + h.ba);
            _x[33] = _lob[21] * _lob[31] * 20.0 / (h.aa + h.ba);
            _x[34] = _lob[0] * _lob[10] * 20.0 / (h.aa + h.ba);
            _x[35] = _lob[1] * _lob[11] * 20.0 / (h.aa + h.ba);
            _x[36] = h.aa;
            _x[37] = h.ba;
            _x[38] = h.aa / h.av / pre_close_price;
            _x[39] = h.ba / h.bv / pre_close_price;
            _x[40] = _x[23] / (_x[36] + _x[37]);
            _x[41] = _rank / 4800.0;
            _x[42] = total_amount / total_volume / pre_close_price;
            _x[43] = _x[4] / pre_close_price - _x[42];
            pld[rank % 5] = _x[43];
            _x[44] = _x[2] - _x[3];
            _x[45] = _x[2] - _x[1];
            _x[46] = _x[3] - _x[1];
            _x[47] = (_lob[20] - _lob[0]) / pre_close_price;
            _x[48] = (_x[38] - _x[39]) / pre_close_price;
            _x[49] = (_lob[20 + h.imav] - _lob[h.imbv]) / pre_close_price;
            if (rank < 5)
                _x[50] = nan("");
            else
                _x[50] = (last_price[rank % 15] + last_price[(15 + rank - 1) % 15] + last_price[(15 + rank - 2) % 15] + last_price[(15 + rank - 3) % 15] + last_price[(15 + rank - 4) % 15]) / 5.0;
            if (rank < 14)
                _x[51] = nan("");
            else
                _x[51] = (last_price[0] + last_price[1] + last_price[2] + last_price[3] + last_price[4] + last_price[5] + last_price[6] + last_price[7] + last_price[8] + last_price[9] + last_price[10] + last_price[11] + last_price[12] + last_price[13] + last_price[14]) / 15.0;
            ma5[rank % 20] = _x[50];
            double _tmp = 0;
            if (rank < 19)
                _x[52] = nan("");
            else
            {
                _tmp = (ma5[0] + ma5[1] + ma5[2] + ma5[3] + ma5[4] + ma5[5] + ma5[6] + ma5[7] + ma5[8] + ma5[9] + ma5[10] + ma5[11] + ma5[12] + ma5[13] + ma5[14] + ma5[15] + ma5[16] + ma5[17] + ma5[18] + ma5[19]) / 20.0;
                _x[52] = sqrt(((ma5[0] - _tmp) * (ma5[0] - _tmp) + (ma5[1] - _tmp) * (ma5[1] - _tmp) + (ma5[2] - _tmp) * (ma5[2] - _tmp) + (ma5[3] - _tmp) * (ma5[3] - _tmp) + (ma5[4] - _tmp) * (ma5[4] - _tmp) + (ma5[5] - _tmp) * (ma5[5] - _tmp) + (ma5[6] - _tmp) * (ma5[6] - _tmp) + (ma5[7] - _tmp) * (ma5[7] - _tmp) + (ma5[8] - _tmp) * (ma5[8] - _tmp) + (ma5[9] - _tmp) * (ma5[9] - _tmp) + (ma5[10] - _tmp) * (ma5[10] - _tmp) + (ma5[11] - _tmp) * (ma5[11] - _tmp) + (ma5[12] - _tmp) * (ma5[12] - _tmp) + (ma5[13] - _tmp) * (ma5[13] - _tmp) + (ma5[14] - _tmp) * (ma5[14] - _tmp) + (ma5[15] - _tmp) * (ma5[15] - _tmp) + (ma5[16] - _tmp) * (ma5[16] - _tmp) + (ma5[17] - _tmp) * (ma5[17] - _tmp) + (ma5[18] - _tmp) * (ma5[18] - _tmp) + (ma5[19] - _tmp) * (ma5[19] - _tmp)) / 20.0);
            }
            if (rank < 4)
            {
                _x[53] = nan("");
                _x[55] = nan("");
            }
            else
            {
                _x[53] = sqrt(((last_price[rank % 15] - _x[50]) * (last_price[rank % 15] - _x[50]) + (last_price[(15 + rank - 1) % 15] - _x[50]) * (last_price[(15 + rank - 1) % 15] - _x[50]) + (last_price[(15 + rank - 2) % 15] - _x[50]) * (last_price[(15 + rank - 2) % 15] - _x[50]) + (last_price[(15 + rank - 3) % 15] - _x[50]) * (last_price[(15 + rank - 3) % 15] - _x[50]) + (last_price[(15 + rank - 4) % 15] - _x[50]) * (last_price[(15 + rank - 4) % 15] - _x[50])) / 5.0);
                _tmp = (pld[0] + pld[1] + pld[2] + pld[3] + pld[4]) / 5.0;
                _x[55] = sqrt(((pld[0] - _tmp) * (pld[0] - _tmp) + (pld[1] - _tmp) * (pld[1] - _tmp) + (pld[2] - _tmp) * (pld[2] - _tmp) + (pld[3] - _tmp) * (pld[3] - _tmp) + (pld[4] - _tmp) * (pld[4] - _tmp)) / 5.0);
            }
            if (rank >= 1)
                _x[56] = last_price[rank % 15] / last_price[(15 + rank - 1) % 15] - 1.0;
            else
                _x[56] = nan("");
            if (rank >= 2)
                _x[57] = last_price[rank % 15] / last_price[(15 + rank - 2) % 15] - 1.0;
            else
                _x[57] = nan("");
            if (rank >= 3)
                _x[58] = last_price[rank % 15] / last_price[(15 + rank - 3) % 15] - 1.0;
            else
                _x[58] = nan("");
            if (rank >= 4)
                _x[59] = last_price[rank % 15] / last_price[(15 + rank - 4) % 15] - 1.0;
            else
                _x[59] = nan("");
            // if (rank < 5)
            // {
            //     printf("Rank = %lu\n, last_price[14] = %.2lf\n", rank, last_price[14]);
            //     printf("Return 1, 2, 3, 4 are %.4lf, %.4lf, %.4lf, %.4lf\n", _x[56], _x[57], _x[58], _x[59]);
            //     printf("Last price 1, 2, 3, 4 are %.4lf, %.4lf, %.4lf, %.4lf\n", last_price[0], last_price[1], last_price[2], last_price[3]);
            //     printf("Last price 1, 2, 3, 4 are %.4lf, %.4lf, %.4lf, %.4lf, %.4lf\n", last_price[rank % 15], last_price[(15 + rank - 1) % 15], last_price[(15 + rank - 2) % 15], last_price[(15 + rank - 3) % 15], last_price[(15 + rank - 4) % 15]);
            // }
            trade_amount = 0;
            trade_no = 0;
            trade_volume = 0;
            ++rank;
        }
        void MemcpyX(double *xx)
        {
            memcpy(xx, _x, sizeof(_x));
        }
    };
}

#endif