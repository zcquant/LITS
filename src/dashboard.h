#ifndef __DASHBOARD__
#define __DASHBOARD__

#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <cassert>
#include <algorithm>
#include <map>
#include <vector>
#include <queue>

#include <stdio.h>
#include <string.h>
#include <math.h>

#include <mpi.h>

#include <Xtech/mode_internal.h>

#include <Xtech/config.h>
#include "utils.h"
#include "orderid.h"
#include "datapoint.h"
#include "factor.h"
// #include "zdice.h"
#include "zspmat.h"
#include "zsupgenus.h"
// #include "zyslice.h"

#include "zxsuperbia.hpp"

// #include <Genus/factors/factor_func.h>
// #include <Genus/factors/factor_helper.h>

// #include "limitorder.hpp"

#include "zxbasic.hpp"

#ifdef ONLINE
#include "LightGBM/c_api.h"
#endif

using namespace std;

class Dashboard
{
public:
    Dashboard();
    ~Dashboard();
    ofstream file_bin_factor;
    void _SET_mpiprocessx(int);
    void set_info(int *_buf_symbol);
    void _test_print();
    void _new_all();
    void on_l2fields(int *fields);
    void get_orderbook();
    void _Snapshot_LOB(int *);
    void set_virtual_order();
    void use_virtual_order();
    void factor_toolbox();
    int symbol, market, rank_process, rank_thread, last_close_price, lowest_price, highest_price, mulx, len_lob;
    double Volume = 0., Amount = 0.;
    int *OFL;
    int process_signal;
    double *features;
    int p_features;
    double vw_atomic, vw_atomic_latest;
    int latest_rank_tick;
    int latest_rank_atomic_calc_vw;
    double _pred[8];

private:
    int mpiProcessX;
    int lob_cut_vw;
    int last_price;
    int latest_rank_check_alive, should_check_alive;
    int vw_atomic_volbid, vw_atomic_volask, vw_atomic_lobbid, vw_atomic_lobask;
    Zqueue<L2DataPointUnstructed> l2dusq;
    Zqueue<L2DataPointStructed> l2dsq;
    Zqueue<L2DataPointMACDs> l2dmacdq;
    int *snapshot_orderbook;
    map<int, Orderid> orderhash;
    Orderqueue *orderqueuebook_bid, *orderqueuebook_ask;
    vector<int> *virtual_orderqueuebook_begin_bid, *virtual_orderqueuebook_begin_ask, *virtual_orderqueuebook_end_bid, *virtual_orderqueuebook_end_ask;
    // ZStudioYSlice _yslice;
#ifdef USING_SUPERBIA
    ZBufferSuperbia *_xbuf_superbia;
#endif

    void _new_orderqueuebook();
    void _new_orderbook();
    void _new_voqb();
    void _new_queues();
    void _new_features();
    void modify_orderqueuebook(int i, int d, int p, int v, int ut, int flag);
    void modify_last_price(int p);
    void modify_vw(int oid0, int oid1, int v, int ut);
    void modify_orderflow(int oid0, int oid1, int p, int v, int ut);
    void set_queues();
    void push_queues(int r);
    int calc_info();
    int calc_yslice();
    int calc_factor_IdxPrice();
    int calc_factor_PWV();
    int calc_factor_MACDs();
    int calc_factor_VolumeFlow();
    int calc_factor_OBDiff();
    int calc_factor_OIR();
    int calc_factor_DELTA();
    int calc_factor_HVUOGLE();
    int calc_factor_DICE();
    int calc_factor_SUPERBIA();

#ifdef SHIYU
    // --- 新增变量 - begin ---
    int *interval_volume; // 记录每个3s间 asklimit、askcancel、askmarket、bidlimit、bidcancel、bidmarket的量
    vector<vector<int>> interval_volume_list{};
    vector<int> MA{}, MB{};          // AP因子用到的三个变量中的两个
    vector<double> TI{};           // AP因子用到的三个变量中的一个
    vector<int> interval_time{};   // 记录订单到来的时间(用于计算时间间隔)
    vector<double> midl{};         // 记录每个3s的mid（最多存储100个mid）、交易金额
    vector<int> amountl{}, numl{};   // 记录每个3s的交易金额（最多存储100个）
    vector<int> interval_amount{}; // 记录每个3s间的逐笔交易金额
    double *oldSigema;           // 存储之前的Sigema1, Sigema2, Alpha, theta0，用于实现ffill
    // --- 新增变量 - end ---

    // --- 新增函数 - begin ---
    // 一个新的订单到来时，更新interval_volume变量
    void update_interval_volume(int id0, int id1, int p, int v);
    // 计算完一轮因子后，清空interval_volume变量
    void clean_interval_volume();
    // 计算因子AskSigma+ 和 Imbalance+
    int calc_factor_AskSigma();

    // 更新计算AP因子需要用到的MA、MB和TI变量，这些变量的计算需要interval_volume
    void update_MAMBTI();
    // 计算因子AP
    int calc_factor_AP();

    // 一个新的订单到来时，更新interval_time变量
    void update_interval_time(int rut);
    // 计算完一轮因子后，清空interval_time变量
    void clean_interval_time();
    // 计算因子Proofdecreasemiuiszero
    int calc_factor_Proofdecreasemiuiszero();

    // 一个新的订单到来时，更新interval_amount变量
    void update_interval_amount(int id0, int id1, int p, int v);
    // 计算完一轮因子后，清空interval_amount变量
    void clean_interval_amount();
    // 需要计算新的一轮因子前，更新midl、amountl、numl
    void update_midl_amountl_numl();
    // 计算因子SR
    int calc_factor_SR();

    // 计算因子alphasum 和 WAB
    int calc_factor_WAB();
    // --- 新增函数 - end ---
#endif /* SHIYU */

#ifdef XBASIC
    lits::ZBoardXBasic *_board_xbasic;
    int calc_factor_xbasic();
#endif
};

#endif