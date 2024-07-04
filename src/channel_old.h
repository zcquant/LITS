#ifndef _CHANNEL_H_
#define _CHANNEL_H_
#include <unordered_set>
#include <vector>
#include <string>

#include "../Includes/Xtech/mode_internal.h"

#ifdef USING_MSGTOOL_T2
// #include "t2sdk_interface.h"
#include <Hundsun/t2sdk/Include/t2sdk_interface.h>
#endif

constexpr auto CLOUD_SIGNAL = "../cloud_signal.xml";

using namespace std;

struct public_test
{
    std::string stock;
    std::string market;
    char bs;
    char oc;
    char pt;
    double price;
    long long volume;
    char time;
    char flag;
    long long signal_id;
    long long timestamp;
};

typedef struct tagT2Info {
    tagT2Info(): lpT2SrvConn(nullptr), lpPublish(nullptr), strTopicName(""), strT2Conf("") {}
    CConnectionInterface* lpT2SrvConn;
    CPublishInterface* lpPublish;
    string strTopicName;
    string strT2Conf;
}T2Info;

class Channel {
public:
    string GetID() const;
    bool Authority() const;
    bool MatchMarket(const string& market) const;
    void SendT2Message(public_test& _SignalData) const;

    bool LoadConf(const void* lp);
    void ToString() const;
    friend bool operator < (const Channel& c1st, const Channel& c2nd);
protected:
    void InitT2(const char* lp);
    void SetID(const char* lp);
    void SetPriority(double db);
    void SetMarket(const char* lp);

    bool GetT2Config(CConfigInterface*& pConfig, std::string sConfigName);
    bool GetT2Connect(CConnectionInterface*& pConnection, CConfigInterface*& pConfig, CCallbackInterface* pCallback);

protected:
    string m_strID{""};
    double m_dbPriority{0.0};
    unordered_set<string> m_usetMarket;
    T2Info m_stT2;
};

class IChannelMgr {
public:
    virtual ~IChannelMgr() = default;
public:
    virtual int Init() = 0;
    virtual int Publish(public_test& _SignalData) = 0;
};


class ChannelManager: public IChannelMgr {
public:
    int Init() override;
    int Publish(public_test& _SignalData) override;

protected:
    int LoadConf();
    void ToString() const;
protected:
    vector<Channel> m_vcChannel;
};

#endif

