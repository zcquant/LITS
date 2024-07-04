#ifndef __ORDERID__
#define __ORDERID__

#include <iostream>
#include <set>

using namespace std;

class Orderid
{
public:
    Orderid();
    ~Orderid();

    int direction;
    int price;
    int volume;
    int price_lock;
    int alive;
    int t_order;
    int t_trade_first;
    int t_trade_last;
    int t_cancel;

    void modify_values(int d, int p, int v, int pl, int al, int tor, int tpt, int tcl, int ll);
    void kill();
    int is_alive();
    void get_pv(int *p, int *v);

private:
};

class Orderqueue
{
public:
    Orderqueue();
    ~Orderqueue();

    int volume;
    int alive_begin_first_use;
    set<int> oq;
    set<int>::iterator alive_begin;

    void add_volume(int d);
    void append_q(int i);
    void remove_q(int i);
    int get_volume();
    set<int> check_alive(int i);
    void get_alive_be(int *i, int *j);
    set<int>::iterator get_oq_alive_begin();
    set<int>::iterator get_oq_alive_end(int i);
    set<int>::iterator get_oq_end();

private:
};

#endif