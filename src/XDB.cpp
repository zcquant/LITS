#include "dashboard.h"
#include "datapoint.h"
#include "factor.h"
#include "orderid.h"

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

Dashboard::Dashboard()
{
    latest_rank_check_alive = 0;
    should_check_alive = 0;
    vw_atomic = 0.0;
    vw_atomic_latest = 0.0;
    latest_rank_tick = 0;
    latest_rank_atomic_calc_vw = 0;
    vw_atomic_volbid = 0;
    vw_atomic_volask = 0;
    vw_atomic_lobbid = 0;
    vw_atomic_lobask = 0;
    l2dusq.n = 0;
    l2dsq.n = 0;
    l2dmacdq.n = 0;
#ifdef USING_SUPERBIA
    _xbuf_superbia = new ZBufferSuperbia(20);
    _xbuf_superbia->NewTimehouse(0);
#endif /* USING_SUPERBIA */
#ifdef SHIYU
    interval_volume = new int[6];
    interval_volume[0] = 0;
    interval_volume[1] = 0;
    interval_volume[2] = 0;
    interval_volume[3] = 0;
    interval_volume[4] = 0;
    interval_volume[5] = 0;
    for (int i = 0; i < 6; i++)
    {
        vector<int> iv;
        interval_volume_list.push_back(iv);
    }
    // interval_volume_list.assign(6, std::vector<int>());
    oldSigema = new double[4];
    oldSigema[0] = 0.;
    oldSigema[1] = 0.;
    oldSigema[2] = 0.;
    oldSigema[3] = 0.;
#endif
#ifdef XBASIC
    _board_xbasic = new lits::ZBoardXBasic();
#endif
}

Dashboard::~Dashboard()
{
    delete[] snapshot_orderbook;
    delete[] orderqueuebook_bid;
    delete[] orderqueuebook_ask;
    delete[] virtual_orderqueuebook_begin_bid;
    delete[] virtual_orderqueuebook_begin_ask;
    delete[] virtual_orderqueuebook_end_bid;
    delete[] virtual_orderqueuebook_end_ask;
    delete[] features;
    delete[] OFL;
#ifdef USING_SUPERBIA
    delete _xbuf_superbia;
#endif /* USING_SUPERBIA */
}

void Dashboard::set_info(int *_buf_symbol)
{
    // std::cout << "Info in setinfo 10" << std::endl;
    // for (int i = 0; i < 8; ++i) std::cout << _buf_symbol[i] << " "; std::cout << std::endl;
    symbol = _buf_symbol[7];
    market = symbol < 600000 ? 0 : 1;
    // std::cout << "Info in setinfo 11" << std::endl;
    rank_process = _buf_symbol[0];
    rank_thread = _buf_symbol[1];
    // std::cout << "Info in setinfo 12" << std::endl;
    last_close_price = _buf_symbol[2];
    lowest_price = _buf_symbol[3];
    // std::cout << "Info in setinfo 13" << std::endl;
    highest_price = _buf_symbol[4];
    mulx = _buf_symbol[5];
    // std::cout << "Info in setinfo 14" << std::endl;
    len_lob = _buf_symbol[6];
    last_price = len_lob;
    // std::cout << "Info in setinfo 15" << std::endl;
    lob_cut_vw = (int)(len_lob * RATE_LOB_CUT_VW) + 1;
    _pred[0] = symbol;
    // std::cout << "Info in setinfo 1" << std::endl;
#ifdef XBASIC
    _board_xbasic->Init(last_close_price * 0.01);
#endif
    // std::cout << "Info in setinfo 2" << std::endl;
    OFL = new int[len_lob];
    // std::cout << "Info in setinfo 3" << std::endl;
    memset(OFL, 0, sizeof(int) * len_lob);
    // std::cout << "Set Info ... " << len_lob << std::endl;
}

void Dashboard::_test_print()
{
    cout << "Dashboard: " << symbol << " " << len_lob << endl;
}

void Dashboard::_SET_mpiprocessx(int p)
{
    mpiProcessX = p;
}

void Dashboard::_new_all()
{
    _new_orderbook();
    _new_orderqueuebook();
    _new_voqb();
    _new_queues();
    _new_features();
}

void Dashboard::_new_orderbook()
{
    snapshot_orderbook = new int[len_lob];
}

void Dashboard::_new_orderqueuebook()
{
    orderqueuebook_bid = new Orderqueue[len_lob + 1];
    orderqueuebook_ask = new Orderqueue[len_lob + 1];
}

void Dashboard::_new_voqb()
{
    virtual_orderqueuebook_begin_bid = new vector<int>[len_lob];
    virtual_orderqueuebook_begin_ask = new vector<int>[len_lob];
    virtual_orderqueuebook_end_bid = new vector<int>[len_lob];
    virtual_orderqueuebook_end_ask = new vector<int>[len_lob];
}

void Dashboard::_new_queues()
{
    l2dsq._new_all(MAX_QUEUE_SIZE_STRUCTED);
    L2DataPointStructed *dps = new L2DataPointStructed;
    dps->rank_tick = 0;
    dps->_new_all(len_lob);
    l2dsq.push(dps);
    l2dusq._new_all(MAX_QUEUE_SIZE_UNSTRUCTED);
    L2DataPointUnstructed *dpus = new L2DataPointUnstructed;
    dpus->rank_tick = 0;
    l2dusq.push(dpus);
    l2dmacdq._new_all(MAX_QUEUE_SIZE_MACDS);
    L2DataPointMACDs *dpmacd = new L2DataPointMACDs;
    dpmacd->rank_tick = 0;
    dpmacd->_new_all();
    l2dmacdq.push(dpmacd);
}

void Dashboard::_new_features()
{
    features = new double[1024];
}

void Dashboard::on_l2fields(int *fields)
{
    int flag = fields[2], _id0 = fields[4], _id1 = fields[5], p = fields[6], v = fields[7], ut = fields[8];
    // int uid = fields[9];
    int rut = FlowUnix(fields[8]);
    int rank_tick = FlowRank(rut, FREQ_TICK);
    // std::cout << "Flag1" << std::endl;
#ifdef SHIYU
    update_interval_amount(_id0, _id1, p, v);
#endif

#ifdef USING_VW
    int rank_atomic_calc_vw = FlowRank(rut, FREQ_ATOMIC_CALC_VW);
    while (latest_rank_atomic_calc_vw < rank_atomic_calc_vw && latest_rank_atomic_calc_vw < 144001)
    {
        if (latest_rank_atomic_calc_vw > 0)
        {
            vw_atomic = vw_atomic + min(vw_atomic_volbid / (vw_atomic_lobbid + 1.0), 1.0) + min(vw_atomic_volask / (vw_atomic_lobask + 1.0), 1.0);
        }
        vw_atomic_volbid = 0;
        vw_atomic_volask = 0;
        vw_atomic_lobbid = 0;
        vw_atomic_lobask = 0;

        int wib = -1, wia = len_lob;
        for (int i = last_price + 1; i < len_lob; i++)
        {
            if (orderqueuebook_ask[i].volume > 0)
            {
                wia = i;
                break;
            }
        }
        for (int i = last_price - 1; i >= 0; i--)
        {
            if (orderqueuebook_bid[i].volume > 0)
            {
                wib = i;
                break;
            }
        }
        for (int i = wib + 1; i < wia; i++)
        {
            if (orderqueuebook_bid[i].volume > 0 && orderqueuebook_ask[i].volume == 0)
            {
                wib = i;
            }
            else if (orderqueuebook_bid[i].volume == 0 && orderqueuebook_ask[i].volume > 0)
            {
                wia = i;
                break;
            }
        }

        int ie;
        ie = min(len_lob, wia + lob_cut_vw);
        for (int i = wia; i < ie; i++)
            vw_atomic_lobask += orderqueuebook_ask[i].volume;
        ie = max(0, wib - lob_cut_vw + 1);
        for (int i = ie; i <= wib; i++)
            vw_atomic_lobbid += orderqueuebook_bid[i].volume;
        latest_rank_atomic_calc_vw++;
    }
#endif
    // std::cout << "Flag2" << std::endl;

    while (latest_rank_tick < rank_tick && latest_rank_tick < 4801)
    {
        /**
         * @brief Tick work ...
         *
         * Work at a fixed frequency (e.g. 3s), to generate factor sequences or other auxiliary data
         *
         */
    // std::cout << "Flag2----" << std::endl;

        set_queues();
    // std::cout << "Flag2====" << std::endl;

#ifdef SHIYU
        update_MAMBTI();
        update_midl_amountl_numl();
#endif
    // std::cout << "Flag2x" << std::endl;

#ifdef XBASIC
        _board_xbasic->RefreshLOB();
        size_t ibp1 = max(l2dusq.back()->idx_bid_1, 0), iap1 = max(l2dusq.back()->idx_ask_1, 0), d = 0;
        auto o = l2dsq.back()->orderbook;
        auto oo = _board_xbasic->_lob;
        std::reverse_iterator<std::set<int>::iterator> pb, pe;
        for (size_t i = ibp1;; --i)
        {
            if (o[i] > 0)
            {
                oo[d] = (i + lowest_price) * 0.01;
                oo[d + 10] = o[i];
                ++d;
                if (d == 1)
                {
                    pb = orderqueuebook_bid[i].oq.rbegin();
                    pe = orderqueuebook_bid[i].oq.rend();
                    for (auto j = pb; j != pe; ++j)
                    {
                        if (orderhash[*j].volume > 0)
                            ++(_board_xbasic->total_bid_order_1);
                        else
                            break;
                    }
                }
                else if (d == 2)
                {
                    pb = orderqueuebook_bid[i].oq.rbegin();
                    pe = orderqueuebook_bid[i].oq.rend();
                    for (auto j = pb; j != pe; ++j)
                    {
                        if (orderhash[*j].volume > 0)
                            ++(_board_xbasic->total_bid_order_2);
                        else
                            break;
                    }
                }
            }
            if (i == 0 || d >= 10)
                break;
        }
        d = 20;
        for (size_t i = iap1; i < len_lob; ++i)
        {
            if (o[i] < 0)
            {
                oo[d] = (i + lowest_price) * 0.01;
                oo[d + 10] = -o[i];
                ++d;
                if (d == 21)
                {
                    pb = orderqueuebook_ask[i].oq.rbegin();
                    pe = orderqueuebook_ask[i].oq.rend();
                    for (auto j = pb; j != pe; ++j)
                    {
                        if (orderhash[*j].volume > 0)
                            ++(_board_xbasic->total_ask_order_1);
                        else
                            break;
                    }
                }
                else if (d == 22)
                {
                    pb = orderqueuebook_ask[i].oq.rbegin();
                    pe = orderqueuebook_ask[i].oq.rend();
                    for (auto j = pb; j != pe; ++j)
                    {
                        if (orderhash[*j].volume > 0)
                            ++(_board_xbasic->total_ask_order_2);
                        else
                            break;
                    }
                }
            }
            if (d >= 30)
                break;
        }
        _board_xbasic->CalcX(latest_rank_tick);
#endif
    // std::cout << "Flag2y" << std::endl;

        factor_toolbox();
    // std::cout << "Flag2z" << std::endl;

#ifdef SHIYU
        clean_interval_volume();
        clean_interval_time();
        clean_interval_amount();
#endif
    // std::cout << "Flag2w" << std::endl;

        latest_rank_tick++;

        push_queues(latest_rank_tick);
    }
    // std::cout << "Flag3" << std::endl;

#ifdef SHIYU
    update_interval_volume(_id0, _id1, p, v);
    // std::cout << "Flagxxx" << std::endl;
    
    update_interval_time(rut);
#endif

    // std::cout << "Flag8" << std::endl;
    int *_fields5 = new int[5];
    switch (flag)
    {
    case 1:
        if (_id1 == 1)
        {
            _fields5[0] = _id0;
            _fields5[1] = 0;
            _fields5[2] = p;
            _fields5[3] = v;
            _fields5[4] = ut;
        }
        else
        {
            _fields5[0] = 0;
            _fields5[1] = _id0;
            _fields5[2] = p;
            _fields5[3] = v;
            _fields5[4] = ut;
        }
        modify_orderqueuebook(_id0, _id1, p, v, ut, 0);
    // std::cout << "Flag3flag10" << std::endl;
#ifdef USING_SUPERBIA
        _xbuf_superbia->point_timehouse[rank_tick].data->_AddValueOnOrder(_id1, p, v);
#endif /* USING_SUPERBIA */
#ifdef XBASIC
        if (lowest_price <= p && p <= highest_price)
        {
            _board_xbasic->ModifyOnOrder(0.01 * p, v, _id1);
        }
#endif
        // _yslice.AddOrder(v, rut);
    // std::cout << "Flag3flag1" << std::endl;
        break;
    case 2:
        _fields5[0] = _id0;
        _fields5[1] = _id1;
        _fields5[2] = p;
        _fields5[3] = v;
        _fields5[4] = ut;
        if (_id0 > 0 && (symbol < 600000 || _id1 == 0 || _id0 < _id1 || fields[8] < 0))
        {
            if (_id1 == 0)
                modify_orderqueuebook(_id0, 1, p, v, ut, 3);
            else if (_id0 < _id1)
                modify_orderqueuebook(_id0, 1, p, v, ut, 2);
            else
                modify_orderqueuebook(_id0, 1, p, v, ut, 1);
        }
        if (_id1 > 0 && (symbol < 600000 || _id0 == 0 || _id1 < _id0 || fields[8] < 0))
        {
            if (_id0 == 0)
                modify_orderqueuebook(_id1, -1, p, v, ut, 3);
            else if (_id0 < _id1)
                modify_orderqueuebook(_id1, -1, p, v, ut, 1);
            else
                modify_orderqueuebook(_id1, -1, p, v, ut, 2);
        }
        if (_id0 > 0 && _id1 > 0)
        {
            modify_last_price(p);
            modify_vw(_id0, _id1, v, rut);
            modify_orderflow(_id0, _id1, p, v, rut);
        }
    // std::cout << "Flag3flag20" << std::endl;
#ifdef USING_SUPERBIA
        if (_id0 != 0 && _id1 != 0)
        {
            if (_id0 > _id1)
            {
                _xbuf_superbia->point_timehouse[rank_tick].data->_AddValueOnTrade(1, p, -v);
                if (symbol >= 600000)
                    _xbuf_superbia->point_timehouse[rank_tick].data->_AddValueOnOrder(1, p, -v);
            }
            else
            {
                _xbuf_superbia->point_timehouse[rank_tick].data->_AddValueOnTrade(-1, p, -v);
                if (symbol < 600000)
                    _xbuf_superbia->point_timehouse[rank_tick].data->_AddValueOnTrade(-1, p, -v);
            }
        }
#endif /* USING_SUPERBIA */
#ifdef XBASIC
        if (lowest_price <= p && p <= highest_price && _id0 > 0 && _id1 > 0)
            _board_xbasic->ModifyOnTrade(0.01 * p, -v);
#endif
        // _yslice.AddTrade(market, -v, rut);
    // std::cout << "Flag3flag2" << std::endl;
        break;
    default:
        break;
    }
}

void Dashboard::set_queues()
{
    get_orderbook();
}

void Dashboard::push_queues(int r)
{
    L2DataPointStructed *dps = new L2DataPointStructed;
    L2DataPointUnstructed *dpus = new L2DataPointUnstructed;
    L2DataPointMACDs *dpmacd = new L2DataPointMACDs;
    dpus->rank_tick = r;
    dps->rank_tick = r;
    dpmacd->rank_tick = r;
    dps->_new_all(len_lob);
    dpmacd->_new_all();
    l2dusq.push(dpus);
    l2dsq.push(dps);
    l2dmacdq.push(dpmacd);
#ifdef USING_SUPERBIA
    _xbuf_superbia->NewTimehouse(r);
#endif
}

void Dashboard::modify_last_price(int p)
{
    if (lowest_price <= p && p <= highest_price)
        last_price = p - lowest_price;
}

void Dashboard::modify_vw(int oid0, int oid1, int v, int ut)
{
    if (oid0 > oid1)
    {
        vw_atomic_volask -= v;
    }
    else
    {
        vw_atomic_volbid -= v;
    }
}

void Dashboard::modify_orderflow(int oid0, int oid1, int p, int v, int ut)
{
    int rt = FlowRank(ut, FREQ_TICK), ip = -1, irts = rt % MAX_QUEUE_SIZE_STRUCTED, irtus = rt % MAX_QUEUE_SIZE_UNSTRUCTED;
    if (lowest_price <= p && p <= highest_price)
    {
        ip = p - lowest_price;
        OFL[ip] -= v;
        Volume -= v;
        Amount -= 0.01 * p * v;
    }
    if (ip != -1 && rt == l2dsq.buf[irts]->rank_tick)
    {
        l2dsq.buf[irts]->orderflow_vol[ip] -= v;
        if (oid0 > oid1)
            l2dsq.buf[irts]->orderflow_bid[ip] -= v;
        else
            l2dsq.buf[irts]->orderflow_ask[ip] -= v;
    }
    if (rt == l2dusq.buf[irtus]->rank_tick)
    {
        if (oid0 > oid1)
            l2dusq.buf[irtus]->delta -= v;
        else
            l2dusq.buf[irtus]->delta += v;
        l2dusq.buf[irtus]->volume -= v;
    }
}

void Dashboard::modify_orderqueuebook(int i, int d, int p, int v, int ut, int flag)
{
    /**
     * @brief modify_orderqueuebook
     * flag:
     *  0 - order
     *  1 - positive trade
     *  2 - negative trade
     *  3 - cancel
     */

    // cout << "Modify oqb on " << i << " " << d << " " << p << " " << v << " " << ut << " " << flag << endl;

    /* modify order queue (basic) >>> */

    map<int, Orderid>::iterator iter = orderhash.find(i), iterc;
    set<int>::iterator iter_oq, iter_oq_last;
    set<int> checklist;
    Orderid o;
    int idx = len_lob, op0 = -1, ov0 = 0, op1 = -1, ov1 = 0;
    if (lowest_price <= p && p <= highest_price)
        idx = p - lowest_price;
    if (iter != orderhash.end())
    {
        o = iter->second;
        o.get_pv(&op0, &ov0);
    }
    switch (flag)
    {
    case 0:
        if (idx == len_lob)
            o.modify_values(d, idx, v, 0, 1, ut, -1, -1, len_lob);
        else
            o.modify_values(d, idx, v, 1, 1, ut, -1, -1, len_lob);
        break;
    case 1:
        o.modify_values(d, idx, v, 0, 1, ut, -1, -1, len_lob);
        break;
    case 2:
        o.modify_values(d, idx, v, 1, 1, -1, ut, -1, len_lob);
        break;
    case 3:
        o.modify_values(d, idx, v, 0, 0, -1, -1, ut, len_lob);
        break;
    default:
        break;
    }
    orderhash[i] = o;
    o.get_pv(&op1, &ov1);
    switch (d)
    {
    case 1:
        if (op0 != -1)
            orderqueuebook_bid[op0].add_volume(max(ov0, 0) * (-1));
        if (op1 != -1)
            orderqueuebook_bid[op1].add_volume(max(ov1, 0));
        if (op0 == -1)
        {
            orderqueuebook_bid[op1].append_q(i);
        }
        else if (op0 != op1)
        {
            orderqueuebook_bid[op0].remove_q(i);
            orderqueuebook_bid[op1].append_q(i);
        }
        break;
    case -1:
        if (op0 != -1)
            orderqueuebook_ask[op0].add_volume(max(ov0, 0) * (-1));
        // cout << "1" << endl;
        if (op1 != -1)
            orderqueuebook_ask[op1].add_volume(max(ov1, 0));
        // cout << "2" << op0 << " " << op1 << " " << len_lob << endl;
        if (op0 == -1)
        {
            // cout << "33" << endl;
            // set<int>::iterator tmp = orderqueuebook_ask[op1].oq.find(i);
            // if (tmp != orderqueuebook_ask[op1].oq.end())
            //     cout << "?!?!" << endl;
            orderqueuebook_ask[op1].append_q(i);
            // cout << "34" << endl;
        }
        else if (op0 != op1)
        {
            orderqueuebook_ask[op0].remove_q(i);
            orderqueuebook_ask[op1].append_q(i);
        }
        // cout << "3" << endl;
        break;
    default:
        break;
    }
}

void Dashboard::_Snapshot_LOB(int *s)
{
    // std::cout << latest_rank_tick << std::endl;
    int limp, ap1 = len_lob, bp1 = -1;
    limp = min((int)((last_price + lowest_price) * 1.010 - lowest_price), len_lob - 1);
    for (int i = limp; i >= 0; i--)
    {
        if (orderqueuebook_bid[i].volume > 0)
        {
            bp1 = i;
            break;
        }
    }
    for (int i = bp1 + 1; i < len_lob; i++)
    {
        if (orderqueuebook_ask[i].volume > 0)
        {
            ap1 = i;
            break;
        }
    }
    if (bp1 > ap1)
    {
        int _tmp = bp1;
        bp1 = ap1;
        ap1 = _tmp;
    }
    memset(s, 0, sizeof(int) * len_lob);
    for (int i = 0; i <= bp1; i++)
        s[i] = orderqueuebook_bid[i].volume;
    for (int i = ap1; i < len_lob; i++)
        s[i] = -orderqueuebook_ask[i].volume;
    // if (latest_rank_tick == 0)
    // {
    //     for (int i = 0; i < len_lob; ++i)
    //         std::cout << orderqueuebook_bid[i].volume - orderqueuebook_ask[i].volume << " ";
    //     std::cout << std::endl;
    // }
}

void Dashboard::get_orderbook()
{
    // std::cout << " Flag glob 0" << std::endl;
    int vol;
    // std::cout << " Flag glob 1" << std::endl;
    L2DataPointUnstructed *p_dpus = l2dusq.back();
    // std::cout << " Flag glob 2" << std::endl;
    L2DataPointStructed *p_dps = l2dsq.back();
    // std::cout << " Flag glob 3" << std::endl;
    p_dpus->idx_bid_1 = -1;
    // std::cout << " Flag glob 4" << std::endl;
    p_dpus->idx_ask_1 = len_lob;
    if (last_price < len_lob && last_price >= 0)
    {
    // std::cout << " Flag glob 51" << std::endl;
        for (int i = 0; i < last_price; i++)
        {
            vol = max(orderqueuebook_bid[i].get_volume(), 0);
            p_dps->orderbook[i] = vol;
            if (vol > 0)
            {
                p_dpus->idx_bid_1 = i;
            }
        }
        for (int i = last_price + 1; i < len_lob; i++)
        {
            vol = max(orderqueuebook_ask[i].get_volume(), 0) * (-1);
            p_dps->orderbook[i] = vol;
            if (vol < 0 && p_dpus->idx_ask_1 == len_lob)
            {
                p_dpus->idx_ask_1 = i;
            }
        }
        int _tmpflaga = p_dpus->idx_ask_1, _tmpflagb = p_dpus->idx_bid_1;
        for (int i = _tmpflagb + 1; i < _tmpflaga; i++)
        {
            if (orderqueuebook_bid[i].volume > 0 && orderqueuebook_ask[i].volume == 0 && p_dpus->idx_ask_1 == _tmpflaga)
            {
                p_dps->orderbook[i] = orderqueuebook_bid[i].volume;
                p_dpus->idx_bid_1 = i;
            }
            else if (orderqueuebook_bid[i].volume == 0 && orderqueuebook_ask[i].volume > 0)
            {
                p_dps->orderbook[i] = orderqueuebook_ask[i].volume * (-1);
                if (p_dpus->idx_ask_1 == _tmpflaga)
                    p_dpus->idx_ask_1 = i;
            }
        }
    }
    else
    {
    // std::cout << " Flag glob 52" << std::endl;
        for (int i = 0; i < len_lob; i++)
        {
            vol = orderqueuebook_bid[i].get_volume() - orderqueuebook_ask[i].get_volume();
            p_dps->orderbook[i] = vol;
            if (vol > 0)
                p_dpus->idx_bid_1 = i;
            else if (vol < 0 && p_dpus->idx_ask_1 == len_lob)
                p_dpus->idx_ask_1 = i;
        }
    }
    p_dpus->idx_mid = 0.5 * (p_dpus->idx_bid_1 + p_dpus->idx_ask_1);
    // const char* fileName = "lob.bin";
    // std::ofstream outFile(fileName, std::ios::binary | std::ios::app);
    // outFile.write(reinterpret_cast<const char*>(&p_dps->orderbook[0]), len_lob * sizeof(int));
    // outFile.close();
}

void Dashboard::set_virtual_order()
{
    int oid0 = 0, oid1 = 0;
    set<int>::iterator iter0, iter1;
    vector<int>::iterator iter0v, iter1v;
    for (int i = 0; i < len_lob; i++)
    {

        iter0 = orderqueuebook_bid[i].oq.begin();
        iter1 = orderqueuebook_bid[i].oq.end();
        if (iter0 == iter1)
        {
            virtual_orderqueuebook_begin_bid[i].push_back(0);
            virtual_orderqueuebook_end_bid[i].push_back(0);
            continue;
        }
        if (virtual_orderqueuebook_begin_bid[i].size() > 0)
        {
            iter0v = virtual_orderqueuebook_begin_bid[i].begin();
            iter1v = virtual_orderqueuebook_begin_bid[i].end();
            while (iter0v != iter1v)
            {
                iter1v--;
                if (*iter1v > 0)
                    iter0 = orderqueuebook_bid[i].oq.find(*iter1v);
                break;
            }
        }
        while (iter0 != iter1)
        {
            if (orderhash.find(*iter0)->second.alive > 0)
            {
                break;
            }
            iter0++;
        }
        if (iter0 != iter1)
        {
            oid0 = *(iter0);
            oid1 = *(--iter1);
            virtual_orderqueuebook_begin_bid[i].push_back(oid0);
            virtual_orderqueuebook_end_bid[i].push_back(oid1);
        }
        else
        {
            virtual_orderqueuebook_begin_bid[i].push_back(0);
            virtual_orderqueuebook_end_bid[i].push_back(0);
        }
    }
}

void Dashboard::use_virtual_order()
{
    vector<int>::iterator iter0, iter1;
    set<int>::iterator iters;
    Orderid o;
    int _flag, rt, j = 0;
    for (int i = 0; i < len_lob; i++)
    {
        cout << "Index i = " << i << ": ";
        iter0 = virtual_orderqueuebook_begin_bid[i].begin();
        iter1 = virtual_orderqueuebook_begin_bid[i].end();
        j = 0;
        while (iter0 != iter1)
        {
            j++;
            if (*iter0 == 0)
            {
                cout << "(No order) ";
                iter0++;
                continue;
            }
            _flag = 0;
            iters = orderqueuebook_bid[i].oq.find(*iter0);
            while (iters != orderqueuebook_bid[i].oq.end())
            {
                o = orderhash.find(*iters)->second;
                if (o.t_trade_last != -1)
                {
                    rt = FlowRank(FlowUnix(o.t_trade_last), FREQ_TICK);
                    if (rt >= j)
                    {
                        cout << o.t_trade_last << "(" << o.t_cancel << ") ";
                        _flag = 1;
                        break;
                    }
                }
                iters++;
            }
            if (_flag == 0)
            {
                cout << "(No trade) ";
            }
            iter0++;
        }
        cout << endl;
    }
}

void Dashboard::factor_toolbox()
{
    for (int i = 0; i < 1024; i++)
        if (i >= 460 || i < 444)
            features[i] = nan("");
    p_features = 0;
    p_features += calc_factor_IdxPrice();
    p_features += calc_factor_PWV();
    p_features += calc_factor_MACDs();
    p_features += calc_factor_VolumeFlow();
    p_features += calc_factor_OBDiff();
    p_features += calc_factor_OIR();
    p_features += calc_factor_DELTA();
    p_features += calc_factor_SUPERBIA();
#ifdef SHIYU
    p_features += calc_factor_AskSigma();
    p_features += calc_factor_AP();
    p_features += calc_factor_Proofdecreasemiuiszero();
    p_features += calc_factor_SR();
    p_features += calc_factor_WAB();
    if (latest_rank_tick < 20)
    {
        for (int i = p_features - 7; i < p_features; ++i)
            features[i] = nan("");
    }
#endif
#ifdef XBASIC
    p_features += calc_factor_xbasic();
#endif
    p_features += calc_info();
}

//  ['spread', 'spreadonmid', 'rank.tick', 'price.b.1', 'price.a.1', 'price.m', 'rank.vick', 'symbol', 'rank.thread', 'utimems']
int Dashboard::calc_info()
{
    double *f = features + p_features;
    int ibp1 = l2dusq.back()->idx_bid_1, iap1 = l2dusq.back()->idx_ask_1;
    double imid = l2dusq.back()->idx_mid;
    *(f++) = 1.0 * (iap1 - ibp1) / last_close_price;
    *(f++) = 1.0 * (iap1 - ibp1) / (imid + lowest_price);
    *(f++) = 1.0 * latest_rank_tick;
    *(f++) = (ibp1 + lowest_price) * 0.01;
    *(f++) = (iap1 + lowest_price) * 0.01;
    *(f++) = (imid + lowest_price) * 0.01;
    *(f++) = vw_atomic;
    *(f++) = symbol;
    *(f++) = rank_thread;
    *(f++) = 3000 * latest_rank_tick;
    return 10;
}

int Dashboard::calc_yslice()
{
    // _yslice.GetValues(features + p_features, latest_rank_tick);
    return 12;
}

int Dashboard::calc_factor_IdxPrice()
{
    double *tmp = new double[MAX_QUEUE_SIZE_UNSTRUCTED];
    int n = l2dusq.size(), k = min(n, l2dusq.max_size), j, pf;
    n = max(n, l2dusq.max_size);
    // cout << "N = " << n << " " << k << endl;
    /* TODO iterator */
    for (int i = 0; i < k; i++)
    {
        j = (n + i) % l2dusq.max_size;
        tmp[i] = l2dusq.buf[j]->idx_mid;
        // l2dusq.push(l2dusq.front());
        // l2dusq.pop();
        // cout << tmp[i] << " ";
    }
    // cout << endl;
    pf = factor_IdxPrice(features + p_features, k - 1, len_lob, tmp);
    delete[] tmp;
    return pf;
}

int Dashboard::calc_factor_PWV()
{
    double *f = features + p_features;
    double *w = new double[len_lob], *o = new double[len_lob];
    double pb = 0.0, pa = 0.0;
    L2DataPointUnstructed *dpus = l2dusq.back();
    int ibp1 = dpus->idx_bid_1, iap1 = dpus->idx_ask_1;
    for (int i = 0; i < len_lob; i++)
    {
        if (i <= ibp1)
            w[i] = i + 1;
        else if (i >= iap1)
            w[i] = len_lob - i;
        else
            w[i] = 0;
        o[i] = l2dsq.back()->orderbook[i];
    }
    if (ibp1 >= 0)
        pb = cblas_ddot(ibp1 + 1, w, 1, o, 1) / cblas_dasum(ibp1 + 1, w, 1);
    if (iap1 < len_lob)
        pa = cblas_ddot(len_lob - iap1, w + iap1, 1, o + iap1, 1) / cblas_dasum(len_lob - iap1, w + iap1, 1);
    if (pb >= 1)
        dpus->pwv_bid = log(pb);
    else
        dpus->pwv_bid = 0.0;
    if (pa <= -1)
        dpus->pwv_ask = (-1) * log((-1) * pa);
    else
        dpus->pwv_ask = 0.0;
    dpus->pwv_imb = dpus->pwv_bid + dpus->pwv_ask;
    delete[] w;
    delete[] o;
    *(f++) = dpus->pwv_bid;
    *(f++) = dpus->pwv_ask;
    *(f++) = dpus->pwv_imb;
    return 3;
}

int Dashboard::calc_factor_MACDs()
{
    double *f = features + p_features;
    double tmp0[5], tmp1[5], nx;
    int n = (l2dmacdq.size() - 2 + MAX_QUEUE_SIZE_MACDS) % MAX_QUEUE_SIZE_MACDS, rt = l2dmacdq.n, pf = 0;
    L2DataPointMACDs *dp = l2dmacdq.buf[n], *dpx = l2dmacdq.back();

    nx = l2dusq.back()->idx_mid / len_lob;
    if (rt == 1)
    {
        tmp1[0] = nx;
        tmp1[1] = nx;
        tmp1[2] = 0.0;
        tmp1[3] = 0.0;
        tmp1[4] = 0.0;
        tmp0[4] = 0.0;
        pf += 6;
    }
    else
    {
        for (int i = 0; i < 5; i++)
        {
            tmp0[i] = dp->macd_idxprice[i];
        }
        pf += factor_MACDs(nx, tmp0, tmp1, params_macd);
    }
    for (int i = 0; i < 5; i++)
    {
        dpx->macd_idxprice[i] = tmp1[i];
        *(f++) = tmp1[i];
    }
    *(f++) = tmp1[4] - tmp0[4];

    nx = l2dusq.back()->pwv_bid;
    if (rt == 1)
    {
        tmp1[0] = nx;
        tmp1[1] = nx;
        tmp1[2] = 0.0;
        tmp1[3] = 0.0;
        tmp1[4] = 0.0;
        tmp0[4] = 0.0;
        pf += 6;
    }
    else
    {
        for (int i = 0; i < 5; i++)
        {
            tmp0[i] = dp->macd_pwv_bid[i];
        }
        pf += factor_MACDs(nx, tmp0, tmp1, params_macd);
    }
    for (int i = 0; i < 5; i++)
    {
        dpx->macd_pwv_bid[i] = tmp1[i];
        *(f++) = tmp1[i];
    }
    *(f++) = tmp1[4] - tmp0[4];

    nx = l2dusq.back()->pwv_ask;
    if (rt == 1)
    {
        tmp1[0] = nx;
        tmp1[1] = nx;
        tmp1[2] = 0.0;
        tmp1[3] = 0.0;
        tmp1[4] = 0.0;
        tmp0[4] = 0.0;
        pf += 6;
    }
    else
    {
        for (int i = 0; i < 5; i++)
        {
            tmp0[i] = dp->macd_pwv_ask[i];
        }
        pf += factor_MACDs(nx, tmp0, tmp1, params_macd);
    }
    for (int i = 0; i < 5; i++)
    {
        dpx->macd_pwv_ask[i] = tmp1[i];
        *(f++) = tmp1[i];
    }
    *(f++) = tmp1[4] - tmp0[4];

    nx = l2dusq.back()->pwv_imb;
    if (rt == 1)
    {
        tmp1[0] = nx;
        tmp1[1] = nx;
        tmp1[2] = 0.0;
        tmp1[3] = 0.0;
        tmp1[4] = 0.0;
        tmp0[4] = 0.0;
        pf += 6;
    }
    else
    {
        for (int i = 0; i < 5; i++)
        {
            tmp0[i] = dp->macd_pwv_imb[i];
        }
        pf += factor_MACDs(nx, tmp0, tmp1, params_macd);
    }
    for (int i = 0; i < 5; i++)
    {
        dpx->macd_pwv_imb[i] = tmp1[i];
        *(f++) = tmp1[i];
    }
    *(f++) = tmp1[4] - tmp0[4];

    return pf;
}

int Dashboard::calc_factor_VolumeFlow()
{
    double *f = features + p_features;
    int rw, tfb, _ttime, ihigh, ilow, iopen, iclose, ioc, ico;
    double sum, sum0, sum1, sum2;
    double *tmp = new double[len_lob];
    double *_idx_price = new double[MAX_QUEUE_SIZE_UNSTRUCTED];
    int n = l2dusq.size(), k = min(n, l2dusq.max_size), j;
    n = max(n, l2dusq.max_size);
    for (int i = 0; i < k; i++)
    {
        j = (n + i) % l2dusq.max_size;
        _idx_price[i] = l2dusq.buf[j]->idx_mid;
    }
    int _time = l2dsq.n - 1;
    // cout << "Before for(nrw)" << endl;
    for (int nrw = 1; nrw < n_rolling_windows - 1; nrw++)
    {
        rw = rolling_windows[nrw];
        // cout << "calc on nrw " << rw << endl;
        tfb = (k - 1) - rw + 1;
        if (tfb >= 0)
        {
            ihigh = (int)(_idx_price[tfb + cblas_idamax(rw, _idx_price + tfb, 1)] + 0.75);
            ilow = (int)(_idx_price[tfb + cblas_idamin(rw, _idx_price + tfb, 1)] + 0.75);
            iopen = (int)(_idx_price[tfb] + 0.75);
            iclose = (int)(_idx_price[k - 1] + 0.75);
            ioc = zblas_imax(iopen, iclose);
            ico = zblas_imin(iopen, iclose);

            // printf("LOW = %d, HIGH = %d, OPEN = %d, CLOSE = %d\n", ilow, ihigh, iopen, iclose);

            memset(tmp, 0, sizeof(double) * len_lob);
            for (int i = 0; i < rw; i++)
            {
                _ttime = (_time - i) % MAX_QUEUE_SIZE_STRUCTED;
                // cout << "check: " << _ttime << " vs " << l2dsq.n << endl;
                for (int j = 0; j < len_lob; j++)
                    tmp[j] += l2dsq.buf[_ttime]->orderflow_bid[j];
            }
            sum = cblas_dasum(ihigh - ilow, tmp + ilow, 1);
            sum0 = cblas_dasum(ico - ilow, tmp + ilow, 1);
            sum1 = cblas_dasum(ioc - ico, tmp + ico, 1);
            sum2 = cblas_dasum(ihigh - ioc, tmp + ioc, 1);
            if (sum > 0)
            {
                *(f++) = sum0 / sum;
                *(f++) = sum1 / sum;
                *(f++) = sum2 / sum;
            }
            *(f++) = zblas_dvwi(len_lob, tmp, 0.0) / len_lob;

            memset(tmp, 0, sizeof(double) * len_lob);
            for (int i = 0; i < rw; i++)
            {
                _ttime = (_time - i) % MAX_QUEUE_SIZE_STRUCTED;
                for (int j = 0; j < len_lob; j++)
                    tmp[j] += l2dsq.buf[_ttime]->orderflow_ask[j];
            }
            sum = cblas_dasum(ihigh - ilow, tmp + ilow, 1);
            sum0 = cblas_dasum(ico - ilow, tmp + ilow, 1);
            sum1 = cblas_dasum(ioc - ico, tmp + ico, 1);
            sum2 = cblas_dasum(ihigh - ioc, tmp + ioc, 1);
            if (sum > 0)
            {
                *(f++) = sum0 / sum;
                *(f++) = sum1 / sum;
                *(f++) = sum2 / sum;
            }
            *(f++) = zblas_dvwi(len_lob, tmp, 0.0) / len_lob;

            memset(tmp, 0, sizeof(double) * len_lob);
            for (int i = 0; i < rw; i++)
            {
                _ttime = (_time - i) % MAX_QUEUE_SIZE_STRUCTED;
                for (int j = 0; j < len_lob; j++)
                    tmp[j] += l2dsq.buf[_ttime]->orderflow_vol[j];
            }
            sum = cblas_dasum(ihigh - ilow, tmp + ilow, 1);
            sum0 = cblas_dasum(ico - ilow, tmp + ilow, 1);
            sum1 = cblas_dasum(ioc - ico, tmp + ico, 1);
            sum2 = cblas_dasum(ihigh - ioc, tmp + ioc, 1);
            if (sum > 0)
            {
                *(f++) = sum0 / sum;
                *(f++) = sum1 / sum;
                *(f++) = sum2 / sum;
            }
            *(f++) = zblas_dvwi(len_lob, tmp, 0.0) / len_lob;
        }
    }
    delete[] tmp;
    delete[] _idx_price;
    return 12 * (n_rolling_windows - 2);
}

int Dashboard::calc_factor_OBDiff()
{
    double *f = features + p_features;
    double *obdiff = new double[len_lob], *w = new double[len_lob];
    double idxmid = l2dusq.back()->idx_mid;
    double maxlen = zblas_dmax(idxmid, len_lob - idxmid), dot, dsum;
    int _time = l2dsq.n - 1;
    for (int i = 0; i < len_lob; i++)
        w[i] = maxlen - zblas_dabs(idxmid - i);
    dsum = cblas_dasum(len_lob, w, 1);
    if (dsum > 0)
    {
        for (int i = 1; i < 11; i++)
        {
            if (_time >= i)
            {
                for (int j = 0; j < len_lob; j++)
                    obdiff[j] = l2dsq.buf[_time % MAX_QUEUE_SIZE_STRUCTED]->orderbook[j] - l2dsq.buf[(_time - i) % MAX_QUEUE_SIZE_STRUCTED]->orderbook[j];
                dot = cblas_ddot(len_lob, w, 1, obdiff, 1) / dsum;
                if (dot > 1)
                    *(f++) = log(dot);
                else if (dot < -1)
                    *(f++) = (-1) * log((-1) * dot);
                else
                    *(f++) = 0.0;
            }
        }
    }
    delete[] obdiff;
    delete[] w;
    return 10;
}

int Dashboard::calc_factor_OIR()
// int factor_OIR(double *_x, int _time, int LOB, int *_ob, int ibp1, int iap1)
{
    double *f = features + p_features;
    int l[5] = {1, 5, 10, (int)(len_lob / 20), (int)(len_lob / 50)};
    int sumlen;
    double *tmp = new double[len_lob];
    double bv, av;
    int _time = l2dsq.n - 1;
    int ibp1 = l2dusq.back()->idx_bid_1, iap1 = l2dusq.back()->idx_ask_1;
    for (int i = 0; i < len_lob; i++)
        tmp[i] = l2dsq.buf[_time % MAX_QUEUE_SIZE_STRUCTED]->orderbook[i];
    if (ibp1 < 0)
    {
        *(f++) = -1.0;
        *(f++) = -1.0;
        *(f++) = -1.0;
        *(f++) = -1.0;
        *(f++) = -1.0;
    }
    else if (iap1 >= len_lob)
    {
        *(f++) = 1.0;
        *(f++) = 1.0;
        *(f++) = 1.0;
        *(f++) = 1.0;
        *(f++) = 1.0;
    }
    else
    {
        for (int i = 0; i < 5; i++)
        {
            sumlen = zblas_imin(l[i], ibp1 + 1);
            bv = cblas_dasum(sumlen, tmp + ibp1 + 1 - sumlen, 1);
            sumlen = zblas_imin(l[i], len_lob - iap1);
            av = cblas_dasum(sumlen, tmp + iap1, 1);
            if (av + bv <= 1)
                *(f++) = 0.0;
            else
                *(f++) = (bv - av) / (bv + av);
        }
    }
    return 5;
}

int Dashboard::calc_factor_DELTA()
// int factor_DELTA(double *_x, int _time, int LOB, double *_deltaacc)
{
    double *f = features + p_features;
    int rw, tfe, tfb;
    double tmp;
    int _time = l2dusq.n - 1;
    if (_time < 1)
        l2dusq.back()->delta_acc = l2dusq.back()->delta;
    else
        l2dusq.back()->delta_acc = l2dusq.back()->delta + l2dusq.buf[(_time - 1) % MAX_QUEUE_SIZE_UNSTRUCTED]->delta_acc;
    for (int nrw = 1; nrw < n_rolling_windows; nrw++)
    {
        rw = rolling_windows[nrw];
        tfe = _time;
        tfb = _time - rw;
        for (int j = 0; j < num_windows; j++)
        {
            if (tfb < -1)
                *(f++) = nan("");
            else
            {
                if (tfb >= 0)
                    tmp = l2dusq.buf[tfe % MAX_QUEUE_SIZE_UNSTRUCTED]->delta_acc - l2dusq.buf[tfb % MAX_QUEUE_SIZE_UNSTRUCTED]->delta_acc;
                else
                    tmp = l2dusq.buf[tfe % MAX_QUEUE_SIZE_UNSTRUCTED]->delta_acc;
                if (tmp > 1)
                    *(f++) = log(tmp);
                else if (tmp < -1)
                    *(f++) = (-1) * log((-1) * tmp);
                else
                    *(f++) = 0.0;
            }
            tfe = tfb;
            tfb -= rw;
        }
    }
    return (n_rolling_windows - 1) * num_windows;
}

int Dashboard::calc_factor_HVUOGLE()
{
    double *f = features + p_features;
    int this_volume = l2dusq.back()->volume, nbigger = 0, nall = min(MAX_QUEUE_SIZE_UNSTRUCTED, l2dusq.n);
    for (int tmpi = 0; tmpi < MAX_QUEUE_SIZE_UNSTRUCTED; tmpi++)
    {
        if (tmpi < l2dusq.n && this_volume < l2dusq.buf[tmpi]->volume)
            nbigger++;
    }
    *(f++) = 1.0 * nbigger / nall;
    if (latest_rank_tick >= 180)
    {
        double change_of_lob = 0.0, deltaacc = 0.0;
        for (int tmpi = 0; tmpi < len_lob; tmpi++)
        {
            change_of_lob += l2dsq.back()->orderflow_vol[tmpi] - l2dsq.buf[(latest_rank_tick - 1) % MAX_QUEUE_SIZE_STRUCTED]->orderflow_vol[tmpi];
        }
        *(f++) = log((this_volume + 1.0) / (abs(change_of_lob) + 1.0));
        *(f++) = log((abs(l2dusq.back()->delta + 1.0) / (change_of_lob + 1.0)));
        // cout << "Check ... " << this_volume << " " << change_of_lob << endl;
        for (int i = 1; i < 9; i++)
        {
            change_of_lob = 0.0;
            for (int tmpi = 0; tmpi < len_lob; tmpi++)
            {
                change_of_lob += l2dsq.back()->orderflow_vol[tmpi] - l2dsq.buf[(l2dusq.n - i * 15) % MAX_QUEUE_SIZE_STRUCTED]->orderflow_vol[tmpi];
            }
            deltaacc = l2dusq.back()->delta_acc - l2dusq.buf[(l2dusq.n - i * 15) % MAX_QUEUE_SIZE_UNSTRUCTED]->delta_acc;
            // cout << "Check ... " << change_of_lob << " " << deltaacc << endl;
            *(f++) = log((abs(deltaacc) + 1.0) / (abs(change_of_lob) + 1.0));
        }
    }
    else
    {
        for (int i = 0; i < 10; i++)
            *(f++) = nan("");
    }
    return 11;
}

int Dashboard::calc_factor_DICE()
{
#ifdef USING_DICE
    double *f = features + p_features;
    *(f++) = dice.amount_ask_backer;
    *(f++) = dice.amount_ask_better;
    *(f++) = dice.amount_ask_market;
    *(f++) = dice.amount_ask_midder;
    *(f++) = dice.amount_bid_backer;
    *(f++) = dice.amount_bid_better;
    *(f++) = dice.amount_bid_market;
    *(f++) = dice.amount_bid_midder;

    dice.RefreshAmount();
    return 8;
#else
    return 0;
#endif
}

int Dashboard::calc_factor_SUPERBIA()
{
#ifdef USING_SUPERBIA
    double *f = features + p_features;
    // cout << "Last rank tick is " << latest_rank_tick << endl;
    // cout << "Test calc superbia: " << l2dusq.back()->idx_bid_1 << " " << l2dusq.back()->idx_ask_1 << endl;
    double _bp1 = 0.01 * (l2dusq.back()->idx_bid_1 + lowest_price), _ap1 = 0.01 * (l2dusq.back()->idx_ask_1 + lowest_price);
    ZPointSuperbia **_tmp = _xbuf_superbia->_buf_superbia;
    double *_lob = _xbuf_superbia->_buf_lob10;
    for (int i = 0; i < 5; ++i)
        _tmp[i] = _xbuf_superbia->point_timehouse[max(0, latest_rank_tick - i)].data;
    _xbuf_superbia->point_timehouse.rbegin()->second.data->_MakeValueWeightedPrice();
    // _xbuf_superbia->point_timehouse.rbegin()->second.data->_Print();
    // cout << "Check1! " << _xbuf_superbia->point_timehouse.rbegin()->second.data->valueWeightedBidOrderPrice << endl;
    _xbuf_superbia->point_timehouse.rbegin()->second.aux1->_MakeValue(_tmp, _bp1, _ap1);
    _xbuf_superbia->point_timehouse.rbegin()->second.aux5->_MakeValue(_tmp, _bp1, _ap1);
    memset(_lob, 0, sizeof(double) * 40);
    int sumb = 0, suma = 0, vol;
    for (int i = l2dusq.back()->idx_bid_1; i >= 0; --i)
    {
        vol = l2dsq.back()->orderbook[i];
        if (vol > 0)
        {
            _lob[sumb] = 0.01 * (i + lowest_price);
            _lob[sumb + 10] = vol;
            ++sumb;
            if (sumb >= 10)
                break;
        }
    }
    for (int i = l2dusq.back()->idx_ask_1; i < len_lob; ++i)
    {
        vol = -(l2dsq.back()->orderbook[i]);
        if (vol > 0)
        {
            _lob[suma + 20] = 0.01 * (i + lowest_price);
            _lob[suma + 30] = vol;
            ++suma;
            if (suma >= 10)
                break;
        }
    }
    _xbuf_superbia->point_timehouse.rbegin()->second.aux1->_MakeValueAmountEq(_lob + 20, _lob + 30, _lob, _lob + 10);
    _xbuf_superbia->point_timehouse.rbegin()->second.aux5->_MakeValueAmountEq(_lob + 20, _lob + 30, _lob, _lob + 10);
    // _xbuf_superbia->point_timehouse.rbegin()->second.aux1->_Print();
    // _xbuf_superbia->point_timehouse.rbegin()->second.aux5->_Print();
    // cout << "Check2! " << _xbuf_superbia->point_timehouse.rbegin()->second.data->valueWeightedBidOrderPrice << endl;
    *(f++) = _xbuf_superbia->point_timehouse.rbegin()->second.data->valueWeightedBidOrderPrice;
    *(f++) = _xbuf_superbia->point_timehouse.rbegin()->second.data->valueWeightedAskOrderPrice;
    *(f++) = _xbuf_superbia->point_timehouse.rbegin()->second.data->valueWeightedBidTradePrice;
    *(f++) = _xbuf_superbia->point_timehouse.rbegin()->second.data->valueWeightedAskTradePrice;
    *(f++) = _xbuf_superbia->point_timehouse.rbegin()->second.aux5->valueBidOrderAmountNorm;
    *(f++) = _xbuf_superbia->point_timehouse.rbegin()->second.aux5->valueAskOrderAmountNorm;
    *(f++) = _xbuf_superbia->point_timehouse.rbegin()->second.aux5->valueBidTradeAmountNorm;
    *(f++) = _xbuf_superbia->point_timehouse.rbegin()->second.aux5->valueAskTradeAmountNorm;
    *(f++) = _xbuf_superbia->point_timehouse.rbegin()->second.aux5->valueBidOrderAmountEq;
    *(f++) = _xbuf_superbia->point_timehouse.rbegin()->second.aux5->valueAskOrderAmountEq;
    *(f++) = _xbuf_superbia->point_timehouse.rbegin()->second.aux5->valueBidTradeAmountEq;
    *(f++) = _xbuf_superbia->point_timehouse.rbegin()->second.aux5->valueAskTradeAmountEq;
    *(f++) = _xbuf_superbia->point_timehouse.rbegin()->second.aux5->valueCrtOrderLog;
    *(f++) = _xbuf_superbia->point_timehouse.rbegin()->second.aux5->valueCrtTradeLog;
    *(f++) = _xbuf_superbia->point_timehouse.rbegin()->second.aux5->valueCrtOrderImbDif;
    *(f++) = _xbuf_superbia->point_timehouse.rbegin()->second.aux5->valueCrtTradeImbDif;
    *(f++) = _xbuf_superbia->point_timehouse.rbegin()->second.aux5->valueBidPriceEq;
    *(f++) = _xbuf_superbia->point_timehouse.rbegin()->second.aux5->valueAskPriceEq;
    *(f++) = _xbuf_superbia->point_timehouse.rbegin()->second.aux1->valueBidOrderAmountEq;
    *(f++) = _xbuf_superbia->point_timehouse.rbegin()->second.aux1->valueAskOrderAmountEq;
    *(f++) = _xbuf_superbia->point_timehouse.rbegin()->second.aux1->valueBidTradeAmountEq;
    *(f++) = _xbuf_superbia->point_timehouse.rbegin()->second.aux1->valueAskTradeAmountEq;
    *(f++) = _xbuf_superbia->point_timehouse.rbegin()->second.aux1->valueCrtOrderLog;
    *(f++) = _xbuf_superbia->point_timehouse.rbegin()->second.aux1->valueCrtTradeLog;
    *(f++) = _xbuf_superbia->point_timehouse.rbegin()->second.aux1->valueCrtOrderImbDif;
    *(f++) = _xbuf_superbia->point_timehouse.rbegin()->second.aux1->valueCrtTradeImbDif;
    *(f++) = _xbuf_superbia->point_timehouse.rbegin()->second.aux1->valueBidPriceEq;
    *(f++) = _xbuf_superbia->point_timehouse.rbegin()->second.aux1->valueAskPriceEq;
    return 28;
#else
    return 0;
#endif /* USING_SUPERBIA */
}

#ifdef SHIYU

void Dashboard::update_interval_volume(int id0, int id1, int p, int v)
{
    // interval_volume[0-5]: volume of asklimit、askcancel、askmarket、bidlimit、bidcancel、bidmarket
    // std::cout << id0 << " " << id1 << " " << p << " " << v << std::endl;
    if (v < 0)
    {
        v = -v;
    }
    if (p == 0)
    {
        if (id0 != 0)
        {
            interval_volume[4] += v;
            interval_volume_list[4].push_back(v);
        }
        else if (id1 != 0)
        {
            interval_volume[1] += v;
            interval_volume_list[1].push_back(v);
        }
    }
    else if (id1 == 1)
    {
        interval_volume[3] += v;
        interval_volume_list[3].push_back(v);
    }
    else if (id1 == -1)
    {
        // std::cout << "?" << std::endl;
        interval_volume[0] += v;
        // std::cout << "??" << std::endl;
        // std::cout << interval_volume_list.size() << std::endl;
        // for (auto &i: interval_volume_list) std::cout << i.size() << std::endl;
        interval_volume_list[0].push_back(v);
        // std::cout << "???" << std::endl;
    }
    else
    {
        if (id0 > id1)
        {
            interval_volume[5] += v;
            interval_volume_list[5].push_back(v);
        }
        else
        {
            interval_volume[2] += v;
            interval_volume_list[2].push_back(v);
        }
    }
}

void Dashboard::clean_interval_volume()
{
    for (int i = 0; i < 6; i++)
    {
        interval_volume[i] = 0;
        interval_volume_list[i].clear();
    }
}

int Dashboard::calc_factor_AskSigma()
{
    // 计算gap
    L2DataPointUnstructed *p_dpus = l2dusq.back();
    L2DataPointStructed *p_dps = l2dsq.back();

    int ap1 = p_dpus->idx_ask_1;
    int bp1 = p_dpus->idx_bid_1;
    int ap2 = 0, bp2 = 0;
    for (int i = ap1 + 1; i < p_dps->len_lob; i++)
    {
        if (p_dps->orderbook[i] != 0)
        {
            ap2 = i;
            break;
        }
    }
    for (int i = bp1 - 1; i >= 0; i--)
    {
        if (p_dps->orderbook[i] != 0)
        {
            bp2 = i;
            break;
        }
    }
    double AskGap1 = 1. * (ap2 - ap1);
    double BidGap1 = 1. * (bp2 - bp1);

    // asklimit、askcancel、askmarket、bidlimit、bidcancel、bidmarket
    double Pbidlimit = interval_volume[3] * 1.0 / (interval_volume[3] + interval_volume[4] + interval_volume[5]);
    double Pbidmarket = interval_volume[5] * 1.0 / (interval_volume[3] + interval_volume[4] + interval_volume[5]);
    double Pasklimit = interval_volume[0] * 1.0 / (interval_volume[0] + interval_volume[1] + interval_volume[2]);
    double Paskmarket = interval_volume[2] * 1.0 / (interval_volume[0] + interval_volume[1] + interval_volume[2]);

    double D = 10.;

    double BidSigmaPlus = pow(8 * D * BidGap1 * Pbidmarket / Pbidlimit + 1, 0.5) - 1;
    double AskSigmaPlus = pow(8 * D * AskGap1 * Paskmarket / Pasklimit + 1, 0.5) - 1;

    double ImbalancePlus = AskSigmaPlus - BidSigmaPlus;

    features[p_features] = AskSigmaPlus;
    features[p_features + 1] = ImbalancePlus;

    return 2;
}

void Dashboard::update_MAMBTI()
{
    // std::cout << "update mambti" << std::endl;
    int ma = interval_volume[0] + interval_volume[2] - interval_volume[1];
    int mb = interval_volume[3] + interval_volume[5] - interval_volume[4];
    L2DataPointUnstructed *p_dpus = l2dusq.back();
    L2DataPointStructed *p_dps = l2dsq.back();
    int bid_vol1 = p_dps->orderbook[p_dpus->idx_bid_1];
    int ask_vol1 = -p_dps->orderbook[p_dpus->idx_ask_1];
    double ti = (bid_vol1 * 1. - ask_vol1) / (bid_vol1 * 1. + ask_vol1 + 1);

    MA.push_back(ma);
    MB.push_back(mb);
    TI.push_back(ti);
    if (MA.size() > 16)
    {
        MA.erase(MA.begin());
        MB.erase(MB.begin());
        TI.erase(TI.begin());
    }
}

int Dashboard::calc_factor_AP()
{
    int ma = 0, mb = 0;
    double ti = 0.;
    for (int i = 0; i < MA.size(); i++)
    {
        ma += MA[i];
        mb += MB[i];
        ti += TI[i];
    }
    ti /= TI.size();

    double *theta = new double[4];
    double *ftheta = new double[4];
    theta[0] = 0.5, theta[1] = 1., theta[2] = 0.5, theta[3] = 1.;
    for (int i = 0; i < 4; i++)
    {
        ftheta[i] = 0.;
    }

    for (int i = 0; i < 60; i++)
    {
        double MAlog = ma * log(1. + 1. / exp(theta[0] + theta[1] * ti) + exp(theta[2] - theta[0] + (theta[3] - theta[1]) * ti));
        double MBlog = mb * log(1. + 1. / exp(theta[2] + theta[3] * ti) + exp(theta[0] - theta[2] + (theta[1] - theta[3]) * ti));
        double H = -MAlog - MBlog;
        double f1x = 1. + 1. / exp(theta[0] + theta[1] * ti) + exp(theta[2] - theta[0] + (theta[3] - theta[1]) * ti);
        double f2x = 1. + 1. / exp(theta[2] + theta[3] * ti) + exp(theta[0] - theta[2] + (theta[1] - theta[3]) * ti);
        double p1 = ma / f1x * (f1x - 1) - mb / f2x * exp(theta[0] - theta[2] + (theta[1] - theta[3]) * ti);
        double p2 = p1 * ti;
        double p3 = mb / f2x * (f2x - 1) - ma / f1x * exp(theta[2] - theta[0] + (theta[3] - theta[1]) * ti);
        double p4 = p3 * ti;

        double pn = sqrt(p1 * p1 + p2 * p2 + p3 * p3 + p4 * p4);
        p1 /= pn, p2 /= pn, p3 /= pn, p4 /= pn;
        ftheta[0] = theta[0] + 0.75 * p1;
        ftheta[1] = theta[1] + 0.75 * p2;
        ftheta[2] = theta[2] + 0.75 * p3;
        ftheta[3] = theta[3] + 0.75 * p4;

        double fMAlog = ma * log(1. + 1. / exp(ftheta[0] + ftheta[1] * ti) + exp(ftheta[2] - ftheta[0] + (ftheta[3] - ftheta[1]) * ti));
        double fMBlog = mb * log(1. + 1. / exp(ftheta[2] + ftheta[3] * ti) + exp(ftheta[0] - ftheta[2] + (ftheta[1] - ftheta[3]) * ti));
        double fH = -fMAlog - fMBlog;

        if (fH > H)
        {
            for (int k = 0; k < 4; k++)
            {
                theta[k] = ftheta[k];
            }
        }
        else
        {
            break;
        }
    }
    double AP = exp(theta[0] + theta[1] * ti) / (exp(theta[0] + theta[1] * ti) + exp(theta[2] + theta[3] * ti));

    features[p_features] = AP;

    return 1;
}

void Dashboard::update_interval_time(int rut)
{
    interval_time.push_back(rut);
}

void Dashboard::clean_interval_time()
{
    while (interval_time.size() > 1)
    {
        interval_time.erase(interval_time.begin());
    }
}

int Dashboard::calc_factor_Proofdecreasemiuiszero()
{
    vector<double> Ds;
    vector<double> Lamdas;
    for (int i = 1; i < interval_time.size(); i++)
    {
        double D = (interval_time[i] - interval_time[i - 1]) / 1000.;
        Ds.push_back(D);
    }
    for (int i = 0; i < Ds.size(); i++)
    {
        double lamda = 100.;
        if (Ds[i] > 0.)
        {
            lamda = 1 / Ds[i];
        }
        Lamdas.push_back(lamda);
    }
    // 计算lamda的方差
    double lamda_mean = 0.;
    for (int i = 0; i < Lamdas.size(); i++)
    {
        lamda_mean += Lamdas[i];
    }
    lamda_mean /= Lamdas.size();
    double lamda_var = 0.;
    for (int i = 0; i < Lamdas.size(); i++)
    {
        lamda_var += (Lamdas[i] - lamda_mean) * (Lamdas[i] - lamda_mean);
    }
    lamda_var /= Lamdas.size();
    // 计算Lamda
    double Lamda = 0.;
    for (int i = 0; i < Ds.size(); i++)
    {
        Lamda += Ds[i];
    }
    Lamda /= Ds.size();
    Lamda = 1 / Lamda;
    // 计算V0-5 记得除以1000
    double *Vs = new double[6];
    for (int i = 0; i < 6; i++)
    {
        Vs[i] = interval_volume[i] / 1000. / Ds.size();
        if (Vs[i] < 0.)
        {
            Vs[i] = -Vs[i];
        }
    }
    // 计算qa和qb
    L2DataPointStructed *p_dps = l2dsq.back();
    int bid1 = -1, ask1 = len_lob;
    for (int i = 0; i < len_lob; i++)
    {
        if (p_dps->orderbook[i] >= 0)
        {
            bid1 += 1;
        }
        else
        {
            break;
        }
    }
    for (int i = bid1; i > 0; i--)
    {
        if (p_dps->orderbook[i] == 0)
        {
            bid1 -= 1;
        }
        else
        {
            break;
        }
    }
    for (int i = len_lob - 1; i >= 0; i--)
    {
        if (p_dps->orderbook[i] <= 0)
        {
            ask1 -= 1;
        }
        else
        {
            break;
        }
    }
    for (int i = ask1; i < len_lob; i++)
    {
        if (p_dps->orderbook[i] == 0)
        {
            ask1 += 1;
        }
        else
        {
            break;
        }
    }
    int bid2 = bid1 - 1, ask2 = ask1 + 1;
    for (int i = bid1 - 1; i > 0; i--)
    {
        if (p_dps->orderbook[i] == 0)
        {
            bid2 -= 1;
        }
        else
        {
            break;
        }
    }
    for (int i = ask1 + 1; i < len_lob; i++)
    {
        if (p_dps->orderbook[i] == 0)
        {
            ask2 += 1;
        }
        else
        {
            break;
        }
    }
    double qa = 0., qb = 0.;
    for (int i = bid1; i > bid1 - 10; i--)
    {
        qb += p_dps->orderbook[i] / 1000.;
    }
    for (int i = ask1; i < ask1 + 10; i++)
    {
        qa -= p_dps->orderbook[i] / 1000.;
    }

    int *trans = new int[6];
    trans[0] = 3, trans[1] = 5, trans[2] = 4, trans[3] = 0, trans[4] = 2, trans[5] = 1;

    double **mA = new double *[6];
    for (int i = 0; i < 6; i++)
    {
        mA[i] = new double[6];
        for (int j = 0; j < 6; j++)
        {
            if (i == j)
            {
                int ind = trans[i];
                double v = 0.;
                for (int k = 0; k < interval_volume_list[ind].size(); k++)
                {
                    v += (interval_volume_list[ind][k] / 1000.) * (interval_volume_list[ind][k] / 1000.);
                }
                v /= Ds.size();
                mA[i][j] = v;
            }
            else
            {
                mA[i][j] = 0.;
            }
        }
    }

    double **mC = new double *[6];
    for (int i = 0; i < 6; i++)
    {
        mC[i] = new double[6];
        for (int j = 0; j < 6; j++)
        {
            mC[i][j] = mA[i][j] - Vs[i] * Vs[j];
        }
    }
    double *V = new double[6];
    for (int i = 0; i < 6; i++)
    {
        V[i] = Vs[trans[i]];
    }

    double Sigema1 = sqrt(Lamda * (lamda_var * V[0] * V[0] + mC[0][0] - 2 * lamda_var * V[1] * V[0] -
                                   2 * mC[0][1] - 2 * lamda_var * V[0] * V[2] - 2 * mC[0][2] + lamda_var * V[1] * V[1] +
                                   mC[1][1] + lamda_var * V[2] * V[2] + mC[2][2] - 2 * lamda_var * V[2] * V[1] - 2 * mC[2][1]));
    double Sigema2 = sqrt(Lamda * (lamda_var * V[3] * V[3] + mC[3][3] - 2 * lamda_var * V[4] * V[3] -
                                   2 * mC[4][3] - 2 * lamda_var * V[3] * V[5] - 2 * mC[3][5] + lamda_var * V[4] * V[4] +
                                   mC[4][4] + lamda_var * V[5] * V[5] + mC[5][5] - 2 * lamda_var * V[5] * V[4] - 2 * mC[5][4]));
    if (std::isnan(Sigema1))
    {
        Sigema1 = oldSigema[0];
    }
    else
    {
        oldSigema[0] = Sigema1;
    }

    if (std::isnan(Sigema2))
    {
        Sigema2 = oldSigema[1];
    }
    else
    {
        oldSigema[1] = Sigema2;
    }

    double Pho = (Lamda * (lamda_var * V[3] * V[0] + mC[0][3] - lamda_var * V[1] * V[3] - mC[1][3] -
                           lamda_var * V[3] * V[2] - mC[2][3] - lamda_var * V[4] * V[0] - mC[0][4] + lamda_var * V[1] * V[4] + mC[1][4] + lamda_var * V[2] * V[4] + mC[2][4] - lamda_var * V[5] * V[0] - mC[0][5] +
                           lamda_var * V[1] * V[5] + mC[1][5] + lamda_var * V[2] * V[5] + mC[2][5])) /
                 (Sigema1 * Sigema2);

    //    if(isnan(Pho)){
    //        Pho = oldSigema[2];
    //    } else{
    //        oldSigema[2] = Pho;
    //    }

    double Alpha = 0.;
    if (abs(Pho) < 0.001)
    {
        Alpha = M_PI;
    }
    else if (Pho > 0)
    {
        Alpha = M_PI + atan(-sqrt(1 - Pho * Pho) / Pho);
    }
    else
    {
        Alpha = atan(-sqrt(1 - Pho * Pho) / Pho);
    }
    if (std::isnan(Alpha))
    {
        Alpha = oldSigema[2];
    }
    else
    {
        oldSigema[2] = Alpha;
    }

    double thetaF = qb / Sigema1 - Pho * qa / Sigema2;
    double theta0 = 0.;
    if (abs(thetaF) < 0.001)
    {
        theta0 = M_PI / 2;
    }
    else if (thetaF > 0)
    {
        theta0 = atan((qa / Sigema2 * sqrt(1 - Pho * Pho)) / (qb / Sigema1 - Pho * qa / Sigema2));
    }
    else
    {
        theta0 = M_PI + atan((qa / Sigema2 * sqrt(1 - Pho * Pho)) / (qb / Sigema1 - Pho * qa / Sigema2));
    }
    if (std::isnan(theta0))
    {
        theta0 = oldSigema[3];
    }
    else
    {
        oldSigema[3] = theta0;
    }

    double Proofdecreasemiuiszero = theta0 / Alpha;
    features[p_features] = Proofdecreasemiuiszero;

    return 1;
}

void Dashboard::update_interval_amount(int id0, int id1, int p, int v)
{
    // std::cout << "interval amount init" << interval_amount.size() << std::endl;
    if ((id1 != 1) && (id1 != -1))
    {
        // if (v < 0)
        // {
        //     v = -v;
        // }
        int amount = p * v;
        // std::cout << amount << " >>> " << interval_amount.size() << std::endl;
        if (p * std::abs(v) > 0)
            interval_amount.push_back(p * std::abs(v));
        // std::cout << amount << " >>> " << interval_amount.size() << std::endl;
    }
    // std::cout << "Init endl!" << std::endl;
}

void Dashboard::clean_interval_amount()
{
    // interval_amount.clear();
    std::vector<int> tmp{};
    interval_amount.swap(tmp);
}

void Dashboard::update_midl_amountl_numl()
{
    // std::cout << "update midl " << std::endl;
    L2DataPointStructed *p_dps = l2dsq.back();
    int bid1 = -1, ask1 = len_lob;
    for (int i = 0; i < len_lob; i++)
    {
        if (p_dps->orderbook[i] >= 0)
        {
            bid1 += 1;
        }
        else
        {
            break;
        }
    }
    for (int i = bid1; i > 0; i--)
    {
        if (p_dps->orderbook[i] == 0)
        {
            bid1 -= 1;
        }
        else
        {
            break;
        }
    }
    for (int i = len_lob - 1; i >= 0; i--)
    {
        if (p_dps->orderbook[i] <= 0)
        {
            ask1 -= 1;
        }
        else
        {
            break;
        }
    }
    for (int i = ask1; i < len_lob; i++)
    {
        if (p_dps->orderbook[i] == 0)
        {
            ask1 += 1;
        }
        else
        {
            break;
        }
    }
    double mid = (bid1 + ask1) / 2. + lowest_price;
    midl.push_back(mid);
    int amount = 0, num = interval_amount.size();
    for (int i = 0; i < interval_amount.size(); i++)
    {
        if (interval_amount[i] >= 0)
        {
            amount += interval_amount[i];
        }
        else
        {
            amount -= interval_amount[i];
        }
    }
    amountl.push_back(amount);
    numl.push_back(num);
    if (midl.size() > 100)
    {
        midl.erase(midl.begin());
        amountl.erase(amountl.begin());
        numl.erase(numl.begin());
    }
}

int Dashboard::calc_factor_SR()
{
    double sr = 0.;
    if (amountl.size() < 26)
    {
        features[p_features] = sr;
        return 1;
    }
    vector<double> avols;
    vector<double> returns;
    for (int i = amountl.size() - 1; i >= amountl.size() - 20; i--)
    {
        double vol_sum = 0.;
        int vol_num = 0;
        for (int j = i; j > i - 6; j--)
        {
            vol_sum += amountl[j] / 100.;
            vol_num += numl[j];
        }
        double avol = vol_sum / vol_num;
        avols.push_back(avol);
        double ret = midl[i] * 1. / midl[i - 6] - 1;
        returns.push_back(ret);
    }

    vector<int> avol_index;
    for (int i = 0; i < avols.size(); i++)
    {
        avol_index.push_back(i);
    }
    stable_sort(avol_index.begin(), avol_index.end(),
         [&avols](int pos1, int pos2)
         { return (avols[pos1] < avols[pos2]); });
    int ret_len = avol_index.size() / 5;
    for (int i = avol_index.size() - 1; i >= avol_index.size() - ret_len; i--)
    {
        sr += returns[avol_index[i]];
    }
    features[p_features] = sr;
    return 1;
}

int Dashboard::calc_factor_WAB()
{
    L2DataPointStructed *p_dps = l2dsq.back();
    int bid1 = -1, ask1 = len_lob;
    for (int i = 0; i < len_lob; i++)
    {
        if (p_dps->orderbook[i] >= 0)
        {
            bid1 += 1;
        }
        else
        {
            break;
        }
    }
    for (int i = bid1; i > 0; i--)
    {
        if (p_dps->orderbook[i] == 0)
        {
            bid1 -= 1;
        }
        else
        {
            break;
        }
    }
    for (int i = len_lob - 1; i >= 0; i--)
    {
        if (p_dps->orderbook[i] <= 0)
        {
            ask1 -= 1;
        }
        else
        {
            break;
        }
    }
    for (int i = ask1; i < len_lob; i++)
    {
        if (p_dps->orderbook[i] == 0)
        {
            ask1 += 1;
        }
        else
        {
            break;
        }
    }
    int bid2 = bid1 - 1, ask2 = ask1 + 1;
    for (int i = bid1 - 1; i > 0; i--)
    {
        if (p_dps->orderbook[i] == 0)
        {
            bid2 -= 1;
        }
        else
        {
            break;
        }
    }
    for (int i = ask1 + 1; i < len_lob; i++)
    {
        if (p_dps->orderbook[i] == 0)
        {
            ask2 += 1;
        }
        else
        {
            break;
        }
    }
    int bid3 = bid2 - 1, ask3 = ask2 + 1;
    for (int i = bid2 - 1; i > 0; i--)
    {
        if (p_dps->orderbook[i] == 0)
        {
            bid3 -= 1;
        }
        else
        {
            break;
        }
    }
    for (int i = ask2 + 1; i < len_lob; i++)
    {
        if (p_dps->orderbook[i] == 0)
        {
            ask3 += 1;
        }
        else
        {
            break;
        }
    }
    int *Bids = new int[3];
    int *Asks = new int[3];
    Bids[0] = p_dps->orderbook[bid1], Bids[1] = p_dps->orderbook[bid2], Bids[2] = p_dps->orderbook[bid3];
    Asks[0] = p_dps->orderbook[ask1], Asks[1] = p_dps->orderbook[ask2], Asks[2] = p_dps->orderbook[ask3];
    int *As = new int[3];
    int *Bs = new int[3];
    As[0] = -Asks[0];
    Bs[0] = Bids[0];
    for (int i = 1; i < 3; i++)
    {
        As[i] = As[i - 1] - Asks[i];
        Bs[i] = Bs[i - 1] + Bids[i];
    }
    double MQ = (ask1 + bid1) / 2.;
    double *RIa = new double[3];
    double *RIb = new double[3];
    for (int i = 0; i < 3; i++)
    {
        RIa[i] = (ask1 + i - MQ) / (MQ + lowest_price);
        RIb[i] = (bid1 - i - MQ) / (MQ + lowest_price);
    }

    double EA = 0., ERIa = 0.;
    double EB = 0., ERIb = 0.;
    double covA = 0., covB = 0.;
    for (int i = 0; i < 3; i++)
    {
        EA += As[i];
        EB += Bs[i];
        ERIa += RIa[i];
        ERIb += RIb[i];
    }
    EA /= 3;
    EB /= 3;
    ERIa /= 3;
    ERIb /= 3;
    for (int i = 0; i < 3; i++)
    {
        covA += (As[i] - EA) * (RIa[i] - ERIa);
        covB += (Bs[i] - EB) * (RIb[i] - ERIb);
    }
    covA /= 2;
    covB /= 2;
    double varA = 0., varB = 0.;
    for (int i = 0; i < 3; i++)
    {
        varA += (As[i] - EA) * (As[i] - EA);
        varB += (Bs[i] - EB) * (Bs[i] - EB);
    }
    varA /= 3;
    varB /= 3;
    double Askbeta = covA / varA;
    double Askalpha = ERIa - Askbeta * EA;
    double Bidbeta = covB / varB;
    double Bidalpha = ERIb - Bidbeta * EB;

    double alphasum = Askalpha + Bidalpha;

    double WA = 0., WB = 0.;
    for (int i = 0; i < 2; i++)
    {
        WA += RIa[i] * Asks[i];
        WB += RIb[i] * Bids[i];
    }
    WA /= As[1];
    WB /= Bs[1];
    double WAB = -WA + WB;
    features[p_features] = alphasum;
    features[p_features + 1] = WAB;

    return 2;
}
#endif /* SHIYU */

#ifdef XBASIC
int Dashboard::calc_factor_xbasic()
{
    double *f = features + p_features;
    _board_xbasic->MemcpyX(f);
    // for (size_t i = 0; i < 60; ++i)
    //     printf("f[%2lu] = %8.4lf\t", i, f[i]);
    // printf("\n");
    return 60;
}
#endif /* XBASIC */

L2DataPointUnstructed::L2DataPointUnstructed()
{
    delta = 0.0;
    volume = 0.0;
}

L2DataPointUnstructed::~L2DataPointUnstructed()
{
}

L2DataPointStructed::L2DataPointStructed()
{
}

L2DataPointStructed::~L2DataPointStructed()
{
    // cout << "BEFORE DELETE DPS" << endl;
    delete[] orderbook;
    // cout << "BEFORE1 DELETE DPS" << endl;
    delete[] orderflow_bid;
    // cout << "BEFORE2 DELETE DPS" << endl;
    delete[] orderflow_ask;
    // cout << "BEFORE3 DELETE DPS" << endl;
    delete[] orderflow_vol;
    // cout << "AFTER DELETE DPS" << endl;
}

void L2DataPointStructed::_new_all(int llob)
{
    len_lob = llob;
    orderbook = new int[len_lob];
    orderflow_bid = new int[len_lob];
    orderflow_ask = new int[len_lob];
    orderflow_vol = new int[len_lob];
    for (int i = 0; i < len_lob; i++)
    {
        orderbook[i] = 0;
        orderflow_bid[i] = 0;
        orderflow_ask[i] = 0;
        orderflow_vol[i] = 0;
    }
}

L2DataPointMACDs::L2DataPointMACDs()
{
}

L2DataPointMACDs::~L2DataPointMACDs()
{
    delete[] macd_idxprice;
    delete[] macd_pwv_bid;
    delete[] macd_pwv_ask;
    delete[] macd_pwv_imb;
}

void L2DataPointMACDs::_new_all()
{
    macd_idxprice = new double[MAX_FIELD_MACD];
    macd_pwv_bid = new double[MAX_FIELD_MACD];
    macd_pwv_ask = new double[MAX_FIELD_MACD];
    macd_pwv_imb = new double[MAX_FIELD_MACD];
}

int zblas_imax(int a, int b)
{
    return a > b ? a : b;
}

int zblas_imin(int a, int b)
{
    return a < b ? a : b;
}

double zblas_dmax(double a, double b)
{
    return a > b ? a : b;
}

double zblas_dmin(double a, double b)
{
    return a < b ? a : b;
}

double zblas_dabs(double a)
{
    return a > 0 ? a : ((-1) * a);
}

double zblas_dstd(const int n, const double *x, const int incx)
{
    double mean = 0.0, std = 0.0;
    for (int i = 0; i < n; i++)
    {
        mean += x[i * incx];
    }
    mean /= n;
    for (int i = 0; i < n; i++)
    {
        std += pow(x[i * incx] - mean, 2);
    }
    std = sqrt(std / n);
    return std;
}

double zblas_dvwi(const int n, const double *x, const double offset)
{
    double ans = nan(""), sum = 0.0;
    double *w = new double[n];
    // w = (double *)malloc(sizeof(double) * n);
    for (int i = 0; i < n; i++)
        w[i] = i + offset;
    sum = cblas_dasum(n, x, 1);
    if (sum > 0)
        ans = cblas_ddot(n, x, 1, w, 1) / sum;
    // free(w);
    delete[] w;
    // w = NULL;
    return ans;
}

int factor_IdxPrice(double *_x, int _time, int LOB, double *_idx_price)
{
    // printf("time is %d\n", _time);
    int _offset_x = 0, rw, rwbox, tfb, tfe, tfb1;
    double max, min, max1, min1;
    // cout << "Still idx price" << endl;
    // for (int i = 0; i < 256; i++)
        // cout << _idx_price[i] << " ";
    // cout << endl;
    for (int i = 0; i < n_rolling_windows; i++)
    {
        tfb = _time - rolling_windows[i];
        if (tfb >= 0)
            _x[i] = _idx_price[tfb] / LOB;
        // else
        //     _x[i] = 0.0;
        // cout << " In calc x[i] " << _x[i] << " " << tfb << " " << _idx_price[tfb] << " " << LOB << endl;
    }
    _offset_x += n_rolling_windows;
    for (int nrw = 1; nrw < n_rolling_windows; nrw++)
    {
        rw = rolling_windows[nrw];
        tfb = _time - rw;
        tfe = _time;
        for (int j = 0; j < num_windows; j++)
        {
            if (tfb >= 0)
                _x[_offset_x + (nrw - 1) * (num_windows + 1) + j] = (_idx_price[tfe] - _idx_price[tfb]) / LOB;
            // else
            //     _x[_offset_x + (nrw - 1) * (num_windows + 1) + j] = 0.0;
            tfe = tfb;
            tfb -= rw;
        }
        if (tfb + rw >= 0)
        {
            _x[_offset_x + (nrw - 1) * (num_windows + 1) + num_windows] = zblas_dstd(num_windows, _x + _offset_x + (nrw - 1) * (num_windows + 1), 1);
        }
        // else
        //     _x[_offset_x + (nrw - 1) * (num_windows + 1) + num_windows] = 0.0;
    }
    _offset_x += (n_rolling_windows - 1) * (num_windows + 1);
    for (int nrw = 1; nrw < n_rolling_windows; nrw++)
    {
        rw = rolling_windows[nrw];
        rwbox = rw * num_windows;
        tfb = _time - rwbox + 1;
        if (tfb >= 0)
        {
            max = _idx_price[tfb + cblas_idamax(rwbox, _idx_price + tfb, 1)];
            min = _idx_price[tfb + cblas_idamin(rwbox, _idx_price + tfb, 1)];
            tfb1 = _time - rw + 1;
            if (max > min)
                for (int j = 0; j < num_windows; j++)
                {
                    max1 = _idx_price[tfb1 + cblas_idamax(rw, _idx_price + tfb1, 1)];
                    min1 = _idx_price[tfb1 + cblas_idamin(rw, _idx_price + tfb1, 1)];
                    _x[_offset_x + (nrw - 1) * (4 * num_windows + 2) + j * 4] = (max1 - min) / (max - min);
                    _x[_offset_x + (nrw - 1) * (4 * num_windows + 2) + j * 4 + 1] = (min1 - min) / (max - min);
                    _x[_offset_x + (nrw - 1) * (4 * num_windows + 2) + j * 4 + 2] = (max1 - min1) / (max - min);
                    _x[_offset_x + (nrw - 1) * (4 * num_windows + 2) + j * 4 + 3] = ((max1 + min1) / 2.0 - min) / (max - min);
                    tfb1 -= rw;
                }
            // else
            //     for (int j = 0; j < num_windows; j++)
            //     {
            //         _x[_offset_x + (nrw - 1) * (4 * num_windows + 2) + j * 4] = nan("");
            //         _x[_offset_x + (nrw - 1) * (4 * num_windows + 2) + j * 4 + 1] = nan("");
            //         _x[_offset_x + (nrw - 1) * (4 * num_windows + 2) + j * 4 + 2] = nan("");
            //         _x[_offset_x + (nrw - 1) * (4 * num_windows + 2) + j * 4 + 3] = nan("");
            //         tfb1 -= rw;
            //     }
            _x[_offset_x + (nrw - 1) * (4 * num_windows + 2) + num_windows * 4] = (max - min) / LOB;
            _x[_offset_x + (nrw - 1) * (4 * num_windows + 2) + num_windows * 4 + 1] = (max + min) / LOB / 2.0;
        }
    }
    _offset_x += (n_rolling_windows - 1) * (4 * num_windows + 2);
    return _offset_x;
}

int factor_MACDs(double x, double *m0, double *m1, const int *par)
{
    m1[0] = x * 2.0 / (par[0] + 1) + m0[0] * (par[0] - 1) / (par[0] + 1);
    m1[1] = x * 2.0 / (par[1] + 1) + m0[1] * (par[1] - 1) / (par[1] + 1);
    m1[2] = m1[0] - m1[1];
    m1[3] = m1[2] * 2.0 / (par[2] + 1) + m0[3] * (par[2] - 1) / (par[2] + 1);
    m1[4] = 2.0 * (m1[2] - m1[3]);
    return 6;
}

Orderid::Orderid()
{
    direction = 0;
    price = 0;
    volume = 0;
    price_lock = 0;
    alive = 1;
    t_order = -1;
    t_trade_first = -1;
    t_trade_last = -1;
    t_cancel = -1;
}

Orderid::~Orderid()
{
}

void Orderid::modify_values(int d, int p, int v, int pl, int al, int tor, int tpt, int tcl, int ll)
{
    if (direction == 0)
    {
        direction = d;
        price = p;
        volume = v;
        price_lock = pl;
        alive = al;
        t_order = tor;
        t_trade_first = tpt;
        t_trade_last = tpt;
        t_cancel = tcl;
    }
    else if (direction == d)
    {
        if (price_lock == 0 && (price == ll || (p != ll && (p - price) * d > 0)))
            price = p;
        volume += v;
        price_lock = pl;
        alive = al;
        if (tor != -1)
            t_order = tor;
        if (tpt != -1)
        {
            if (t_trade_first == -1)
                t_trade_first = tpt;
            if (t_trade_last < tpt)
                t_trade_last = tpt;
        }
        if (tcl != -1)
        {
            t_cancel = tcl;
        }
    }
    if (t_cancel != -1 || (t_order != -1 && volume == 0))
        alive = 0;
}

void Orderid::kill()
{
    alive = 0;
}

int Orderid::is_alive()
{
    return alive;
}

void Orderid::get_pv(int *p, int *v)
{
    if (alive > 0)
    {
        *p = price;
        *v = volume;
    }
    else
    {
        *p = price;
        *v = 0;
    }
}

Orderqueue::Orderqueue()
{
    volume = 0;
    alive_begin = oq.begin();
    alive_begin_first_use = 1;
}

Orderqueue::~Orderqueue()
{
}

void Orderqueue::add_volume(int d)
{
    volume += d;
}

void Orderqueue::append_q(int i)
{
    // cout << "before append" << endl;
    oq.insert(i);
    // cout << "after append" << endl;
}

void Orderqueue::remove_q(int i)
{
    oq.erase(i);
}

int Orderqueue::get_volume()
{
    return volume;
}

set<int>::iterator Orderqueue::get_oq_alive_begin()
{
    if (alive_begin_first_use == 1)
    {
        return oq.begin();
        if (oq.size() > 0)
        {
            alive_begin = oq.begin();
            alive_begin_first_use = 0;
        }
    }
    return alive_begin;
}

set<int>::iterator Orderqueue::get_oq_alive_end(int i)
{
    return oq.lower_bound(i);
}

set<int>::iterator Orderqueue::get_oq_end()
{
    return oq.end();
}

set<int> Orderqueue::check_alive(int i)
{
    set<int> checklist;
    set<int>::iterator iter0 = get_oq_alive_begin(), iter1 = get_oq_alive_end(i);

    if (*iter0 == 0 || oq.size() == 0)
        return checklist;
    while (iter0 != iter1)
    {
        if (*iter0 < i)
            checklist.insert(*iter0);
        else
            break;
        iter0++;
    }
    alive_begin = iter1;
    return checklist;
}

void Orderqueue::get_alive_be(int *i, int *j)
{
}
