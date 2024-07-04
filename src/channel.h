#ifndef _CHANNEL_H_
#define _CHANNEL_H_
#include <unordered_set>
#include <vector>
#include <string>

#include "../Includes/Xtech/mode_internal.h"

// #ifdef USING_MSGTOOL_T2
// #include "t2sdk_interface.h"
#include <Hundsun/t2sdk/Include/t2sdk_interface.h>
// #endif

constexpr auto CLOUD_SIGNAL = "cloud_signal.xml";

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

class CCallbackImpl : public CCallbackInterface
{
public:
    unsigned long  FUNCTION_CALL_MODE QueryInterface(const char* iid, IKnown** ppv);
    unsigned long  FUNCTION_CALL_MODE AddRef();
    unsigned long  FUNCTION_CALL_MODE Release();
    void FUNCTION_CALL_MODE OnConnect(CConnectionInterface* lpConnection);
    void FUNCTION_CALL_MODE OnSafeConnect(CConnectionInterface* lpConnection);
    void FUNCTION_CALL_MODE OnRegister(CConnectionInterface* lpConnection);
    void FUNCTION_CALL_MODE OnClose(CConnectionInterface* lpConnection);
    void FUNCTION_CALL_MODE OnSent(CConnectionInterface* lpConnection, int hSend, void* reserved1, void* reserved2, int nQueuingData);
    void FUNCTION_CALL_MODE Reserved1(void* a, void* b, void* c, void* d);
    void FUNCTION_CALL_MODE Reserved2(void* a, void* b, void* c, void* d);
    int  FUNCTION_CALL_MODE Reserved3();
    void FUNCTION_CALL_MODE Reserved4();
    void FUNCTION_CALL_MODE Reserved5();
    void FUNCTION_CALL_MODE Reserved6();
    void FUNCTION_CALL_MODE Reserved7();
    void FUNCTION_CALL_MODE OnReceivedBiz(CConnectionInterface* lpConnection, int hSend, const void* lpUnPackerOrStr, int nResult);
    void FUNCTION_CALL_MODE OnReceivedBizEx(CConnectionInterface* lpConnection, int hSend, LPRET_DATA lpRetData, const void* lpUnpackerOrStr, int nResult);
    void FUNCTION_CALL_MODE OnReceivedBizMsg(CConnectionInterface* lpConnection, int hSend, IBizMessage* lpMsg);
    ~CCallbackImpl() {}
};

typedef struct tagT2Info {
    tagT2Info(): lpT2SrvConn(nullptr), lpPublish(nullptr), strTopicName(""), strT2Conf("") {}
    CConnectionInterface* lpT2SrvConn;
    CPublishInterface* lpPublish;
    CCallbackInterface* lpCallback;
    string strTopicName;
    string strT2Conf;
}T2Info;

class Channel {
public:
    string GetID() const;
    bool IsVerbose() const;
    string GetSignalName() const;
    bool Authority() const;
    bool MatchMarket(const string& market) const;
    void SendT2Message(public_test& _SignalData) const;
    int T2_PublishData(
        char* szTopicName,
        char* szSignalName,
        char* szStockCode,
        char* szMarketNo,
        char cEnTrustBS,
        char cEnTrustOC,
        char cEnTrustPriceType,
        double fEnTrustPrice,
        long long iEnTrustAmount,
        char cValidTimes,
        char cTradeFlag,
        long long isignal_id) const;

    bool LoadConf(const void* lp);
    void ToString() const;
    void ReleaseT2();
    friend bool operator < (const Channel& c1st, const Channel& c2nd);
protected:
    int InitT2(const char* lp);
    void SetID(const char* lp);
    void SetPriority(double db);
    void SetMarket(const char* lp);
    void SetVerbose(const int n);
    void SetSignalName(const char* lp);

    bool GetT2Config(CConfigInterface*& pConfig, std::string sConfigName);
    bool GetT2Connect(CConnectionInterface*& pConnection, CConfigInterface*& pConfig, CCallbackInterface* pCallback);

protected:
    string m_strID{""};
    double m_dbPriority{0.0};
    unordered_set<string> m_usetMarket;
    T2Info m_stT2;
    bool m_bVerbose{false};
    string m_strSignalName{ "" };
};

class IChannelMgr {
public:
    virtual ~IChannelMgr() = default;
public:
    virtual int Init() = 0;
    virtual int Exit() = 0;
    virtual int Publish(public_test& _SignalData) = 0;
    virtual int Publish2(
        char* szTopicName,
        char* szSignalName,
        char* szStockCode,
        char* szMarketNo,
        char cEnTrustBS,
        char cEnTrustOC,
        char cEnTrustPriceType,
        double fEnTrustPrice,
        long long iEnTrustAmount,
        char cValidTimes,
        char cTradeFlag,
        long long isignal_id) = 0;
};


class ChannelManager: public IChannelMgr {
public:
    int Init() override;
    int Exit() override;
    int Publish(public_test& _SignalData) override;
    int Publish2(
        char* szTopicName,
        char* szSignalName,
        char* szStockCode,
        char* szMarketNo,
        char cEnTrustBS,
        char cEnTrustOC,
        char cEnTrustPriceType,
        double fEnTrustPrice,
        long long iEnTrustAmount,
        char cValidTimes,
        char cTradeFlag,
        long long isignal_id) override;

protected:
    int LoadConf();
    void ToString() const;
protected:
    vector<Channel> m_vcChannel;
};

#endif

