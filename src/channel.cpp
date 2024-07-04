#include <iostream>     // std::cout
#include <iterator>     // std::ostream_iterator
#include <algorithm>    // std::copy
#include <sstream>      // std::ostringstream
// #include "t2sdk_interface.h"
#include "channel.h"
#include "tinyxml2.h"

using namespace tinyxml2;
using std::cout;
using std::cerr;
using std::endl;

int ChannelManager::Init()
{
    int ret = -1;
    ret = LoadConf();
    if (0 != ret) {
        cerr << "Load config file, err:" << ret << endl;
        return ret;
    }

    std::stable_sort(m_vcChannel.begin(), m_vcChannel.end());

    cout << "Channel manager initialized" << endl;
    ToString();

    return ret;
}

int ChannelManager::Exit()
{
    for (auto& c : m_vcChannel) {
        c.ReleaseT2();
    }
    return 0;
}

int ChannelManager::Publish(public_test& _SignalData)
{
    for (const auto & c: m_vcChannel) {
        if (!c.MatchMarket(_SignalData.market)) {
            if (c.IsVerbose()) {
                cout << "Match none market, signal_id: " << _SignalData.signal_id << ", market: " << _SignalData.market << ", channel: " << c.GetID() << endl;
            }
            continue;
        }
        c.SendT2Message(_SignalData);

        if (c.IsVerbose()) {
            cout << "Published, signal_id:" << _SignalData.signal_id << ", market: " << _SignalData.market << ", channel: " << c.GetID() << endl;
        }
    }
    return 0;
}

int ChannelManager::Publish2(
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
    long long isignal_id)
{
    for (const auto& c : m_vcChannel) {
        if (!c.MatchMarket(szMarketNo)) {
            if (c.IsVerbose()) {
                cout << "Match none market, signal_id:" << isignal_id << ", market: " << szMarketNo << ", channel: " << c.GetID() << endl;
            }
            continue;
        }
        c.T2_PublishData(
            szTopicName, 
            szSignalName, 
            szStockCode, 
            szMarketNo, 
            cEnTrustBS,
            cEnTrustOC, 
            cEnTrustPriceType, 
            fEnTrustPrice, 
            iEnTrustAmount, 
            cValidTimes, 
            cTradeFlag, 
            isignal_id);
        if (c.IsVerbose()) {
            cout << "Publish, signal_id:" << isignal_id << ", market: " << szMarketNo << ", channel: " << c.GetID() << endl;
        }
    }
    return 0;
}

void ChannelManager::ToString() const {
    for (const auto& c : m_vcChannel) {
        if (c.IsVerbose()) {
            c.ToString();
        }
        else {
            cout << "Channel:" << c.GetID() << ", verbose: " << boolalpha << c.IsVerbose() << endl;
        }
    }
}

int ChannelManager::LoadConf() {
    string strFile{ CLOUD_SIGNAL };
    XMLDocument cfg;
    XMLError err = cfg.LoadFile(strFile.c_str());
    if (err != XML_SUCCESS) {
        cerr << "配置文件: " << strFile.c_str() << ", 不存在或格式错误, 请检查后重试" << endl;
        return -1;
    }

    XMLElement* root = cfg.RootElement();
    if (!root) {
        cerr << "读取根节点root失败" << endl;
        return -2;
    }

    XMLElement* element = root->FirstChildElement("element");
    while (element) {
        Channel oChannel;
        if (!oChannel.LoadConf(element)) {
            cerr << "Load channel failed" << endl;
        }
        else {
            if (oChannel.Authority()) {
                m_vcChannel.push_back(oChannel);
            }
            else {
                cerr << "Authority failed, channel: " << oChannel.GetID() << endl;
            }
        }

        element = element->NextSiblingElement("element");
    }

    cout << "Channel manager loaded config, file: " << strFile << endl;

    return 0;
}

int Channel::InitT2(const char* lp) {
    m_stT2.strT2Conf = lp;
    CCallbackInterface*& lpCallback = m_stT2.lpCallback;
    string& sTopicName = m_stT2.strTopicName;
    CPublishInterface*& lpPublish = m_stT2.lpPublish;
    CConnectionInterface*& lpT2SrvConn = m_stT2.lpT2SrvConn;
    if (lpT2SrvConn == nullptr) {
        CConfigInterface* pConfig = nullptr;
        if (!GetT2Config(pConfig, m_stT2.strT2Conf.c_str())) {
            std::cout << "读取配置文件" << m_stT2.strT2Conf << "失败，请检查！" << std::endl;
            return -1;
        }

        //创建自定义类CCallback的对象（在创建连接时需传递此对象，请看下面代码）
        lpCallback = new CCallbackImpl();
        lpCallback->AddRef();
        if (!GetT2Connect(lpT2SrvConn, pConfig, lpCallback)) {
            std::cout << "创建T2 连接失败！" << std::endl;
            return -2;
        }

        int nPort = -1;
        const char* lpIp = lpT2SrvConn->GetServerAddress(&nPort);
        std::cout << "创建T2连接成功！" << " server: " << lpIp << ":" << nPort << std::endl;

        if (lpPublish == nullptr) {
            char* clientName = (char*)pConfig->GetString("mc", "client_name", "xtech");//获取发布的主题名
            sTopicName = pConfig->GetString("publish", "topic_name", "stp.jygt.signalpush");
            lpPublish = lpT2SrvConn->NewPublisher(clientName, 200, 5000);
            if (lpPublish == nullptr) {
                std::cout << "创建T2消息发布者失败，失败原因[" << lpT2SrvConn->GetMCLastError() << "]" << std::endl;
                pConfig->Release();
                lpT2SrvConn->Release();
                lpT2SrvConn = nullptr;
                return -3;
            }

            lpPublish->AddRef();
            std::cout << "创建T2消息发布者成功! topic: " << sTopicName << std::endl;
        }
    }

    return 0;
}

inline string Channel::GetID() const {
    return m_strID;
}

bool Channel::Authority() const {
    cout << "Authority passed, channel: " << m_strID << endl;
    return true;
}

bool Channel::MatchMarket(const string& market) const {
    return m_usetMarket.find(market) != m_usetMarket.end();
}

void Channel::SetID(const char* lp) {
    m_strID = lp;
}

void Channel::SetPriority(double db) {
    m_dbPriority = db;
}

void Channel::SetMarket(const char* lp) {
    size_t pos = 0, found = 0;
    string str(lp);
    while (found != string::npos) {
        found = str.find("|", pos);
        m_usetMarket.insert(std::string(str, pos, found - pos));
        pos = found + 1;
    }
}

void Channel::SetVerbose(const int n) {
    m_bVerbose = (n == 0) ? false : true;
}

inline bool Channel::IsVerbose() const {
    return m_bVerbose;
}

void Channel::SetSignalName(const char* lp) {
    m_strSignalName = lp;
}

inline string Channel::GetSignalName() const {
    return m_strSignalName;
}

bool Channel::GetT2Config(CConfigInterface*& pConfig, std::string sConfigName) {
    pConfig = NewConfig();
    if (pConfig == nullptr) {
        std::cout << "NewConfig Memory Allocate Failed";
        return false;
    }

    pConfig->AddRef();
    if (pConfig->Load(sConfigName.c_str()) != 0) {
        std::cout << "Load Configure File Failed, ConfigPath[" << sConfigName << "]";

        pConfig->Release();
        pConfig = nullptr;
        return false;
    }

    return true;
}

bool Channel::GetT2Connect(CConnectionInterface*& pConnection, CConfigInterface*& pConfig, CCallbackInterface* pCallback) {
    pConnection = NewConnection(pConfig);
    if (pConnection == nullptr) {
        std::cout << "NewConnection Memory Allocate Failed";
        pConfig->Release();
        pConfig = nullptr;
        return false;
    }

    int ret = 0;
    pConnection->AddRef();
    //创建自定义类CCallback的对象（在创建连接时需传递此对象，请看下面代码）     

    //初始化连接对象，返回0表示初始化成功，注意此时并没开始连接服务器
    if (0 == (ret = pConnection->Create2BizMsg(pCallback))) {
        //正式开始连接，参数CONNECT_TIME_OUT为超时参数，单位是ms
        if (0 != (ret = pConnection->Connect(3000))) {
            std::cout << "T2 连接超时，错误信息:" << pConnection->GetErrorMsg(ret);

            pConfig->Release();
            pConfig = nullptr;
            pConnection->Release();
            pConnection = nullptr;

            return false;
        }
    }
    else {
        std::cout << "T2 连接失败，错误信息:" << pConnection->GetErrorMsg(ret);

        pConfig->Release();
        pConfig = nullptr;
        pConnection->Release();
        pConnection = nullptr;

        return false;
    }

    return true;
}

bool Channel::LoadConf(const void* lp) {
    XMLElement* element = (XMLElement*)lp;
    XMLElement* temp = element->FirstChildElement("id");
    if (temp == nullptr) {
        cerr << "id必须存在" << endl;
        return false;
    }
    else {
        SetID(temp->GetText());
    }

    temp = element->FirstChildElement("priority");
    if (temp == nullptr) {
        cerr << "priority必须存在" << endl;
        return false;
    }
    else {
        SetPriority(temp->DoubleText());
    }

    temp = element->FirstChildElement("market");
    if (temp == nullptr) {
        cerr << "market必须存在" << endl;
        return false;
    }
    else {
        SetMarket(temp->GetText());
    }

    temp = element->FirstChildElement("verbose");
    if (temp == nullptr) {
        cerr << "verbose必须存在" << endl;
        return false;
    }
    else {
        SetVerbose(temp->IntText());
    }

    temp = element->FirstChildElement("signal_name");
    if (temp == nullptr) {
        cerr << "signal_name必须存在" << endl;
        return false;
    }
    else {
        SetSignalName(temp->GetText());
    }

    temp = element->FirstChildElement("t2_config");
    if (temp == nullptr) {
        cerr << "t2_config必须存在" << endl;
        return false;
    }
    else {
        int ret = InitT2(temp->GetText());
        if (0 != ret) {
            cerr << "Initialized t2 faied, err: " << ret << endl;
            return false;
        }
    }

    cout << "Channel loaded config, id: " << m_strID << endl;

    return true;
}

void Channel::ReleaseT2() {
    if (m_stT2.lpCallback) {
        m_stT2.lpCallback->Release();
        m_stT2.lpCallback = nullptr;
    }
    if (m_stT2.lpT2SrvConn) {
        m_stT2.lpT2SrvConn->Release();
        m_stT2.lpT2SrvConn = nullptr;
    }
}

void Channel::ToString() const {
    std::ostringstream oss;
    oss << "m_strID: " << m_strID << endl;
    oss << "m_dbPriority: " << m_dbPriority << endl;
    std::ostream_iterator<std::string> os_it(oss << "m_usetMarket: ", "|");
    std::copy(m_usetMarket.begin(), m_usetMarket.end(), os_it);
    oss << endl;
    oss << "m_bVerbose: " << boolalpha << m_bVerbose << endl;
    oss << "m_strSignalName: " << m_strSignalName << endl;
    oss << "m_stT2.strT2Conf: " << m_stT2.strT2Conf << endl;
    oss << "m_stT2.strTopicName: " << m_stT2.strTopicName << endl;
    oss << "m_stT2.lpPublish: " << m_stT2.lpPublish << endl;
    oss << "m_stT2.lpT2SrvConn: " << m_stT2.lpT2SrvConn << endl;
    oss << "m_stT2.lpCallback: " << m_stT2.lpCallback << endl;

    cout << oss.str();
}

int Channel::T2_PublishData(
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
    long long isignal_id) const {
    //TopicName	XTECH.T0.TRADE.SIGNAL
    //字段内容	示例	类型及说明
    //股票代码	9	Int
    //交易方向	0	Int, 0为买，1为卖
    //委托方式	1	Int, 0为市价，1为即时对手方最优价，2为限价
    //委托价格	0	Int, 对于股票，使用原价格 * 100
    //昨日收盘价	0	Int, 对于股票，使用原价格 * 100
    //委托量	100	Int
    //持续时间 / 秒	60	Int, / s, 该时间后平仓
    //交易规则标识	0	Int
    string strTopicName;
    if (0 == strlen(szTopicName)) {
        strTopicName = m_stT2.strTopicName;
        if (IsVerbose()) {
            cout << "Set topic name from: " << szTopicName << ", to: " << strTopicName << endl;
        }
    }
    else {
        strTopicName = szTopicName;
    }

    string strSignalName;
    if (0 == strlen(szSignalName)) {
        strSignalName = GetSignalName();
        if (IsVerbose()) {
            cout << "Set signal name from: " << szSignalName << ", to: " << strSignalName << endl;
        }
    }
    else {
        strSignalName = szSignalName;
    }

    //构造发布的业务包内容
    IF2Packer* lpOnePack = NewPacker(2);
    lpOnePack->AddRef();
    lpOnePack->BeginPack();
    lpOnePack->AddField("signal_name", 'S', 16);
    lpOnePack->AddField("stock_code", 'S', 6);
    lpOnePack->AddField("market_no", 'S', 4);
    lpOnePack->AddField("entrust_bs", 'C', 1);
    lpOnePack->AddField("entrust_oc", 'C', 1);
    lpOnePack->AddField("entrust_price_type", 'C', 1);
    lpOnePack->AddField("entrust_price", 'D', 11, 4);
    lpOnePack->AddField("entrust_amount", 'I', 10);
    lpOnePack->AddField("valid_times", 'C', 1);
    lpOnePack->AddField("trade_flag", 'C', 1);
    lpOnePack->AddField("signal_id", 'I', 10);

    lpOnePack->AddStr(strSignalName.c_str()); //signal_name
    lpOnePack->AddStr(szStockCode); //stock_code
    lpOnePack->AddStr(szMarketNo); //market_no
    lpOnePack->AddChar(cEnTrustBS); //entrust_direction
    lpOnePack->AddChar(cEnTrustOC); //entrust_direction
    lpOnePack->AddChar(cEnTrustPriceType); //entrust_price_type
    lpOnePack->AddDouble(fEnTrustPrice); //entrust_price
    lpOnePack->AddInt(iEnTrustAmount); //entrust_amount
    lpOnePack->AddChar(cValidTimes); //valid_times
    lpOnePack->AddChar(cTradeFlag); //trade_flag
    lpOnePack->AddInt(m_stT2.lpPublish->GetMsgNoByTopicName((char*)strTopicName.c_str())); //signal_id

    lpOnePack->EndPack();

    IF2UnPacker* lpUnPack = lpOnePack->UnPack();

    int nCount = 0;
    int iRet = 0;
    while (1) {
        //业务包构造完毕
        //调用业务的发送接口进行发布
        iRet = m_stT2.lpPublish->PubMsgByPacker((char*)strTopicName.c_str(), lpUnPack, 5000, NULL);
        //打印错误信息
        if (IsVerbose()) {
            printf("Next %d, %s, MsgID:%d, topic:%s\n", 
                iRet, 
                m_stT2.lpT2SrvConn->GetErrorMsg(iRet), 
                m_stT2.lpPublish->GetMsgNoByTopicName((char*)strTopicName.c_str()), 
                strTopicName.c_str());
        }

        if (iRet != 0) {
            nCount++;
            if (nCount > 3) {
                break;
            }
        }
        else {
            break;
        }
    }

    lpOnePack->FreeMem(lpOnePack->GetPackBuf());
    lpOnePack->Release();
    return iRet;
}

void Channel::SendT2Message(public_test& _SignalData) const
{
    IF2Packer* lpOnePack = NewPacker(2);
    lpOnePack->AddRef();
    lpOnePack->BeginPack();
    lpOnePack->AddField("signal_name", 'S', 16);
    lpOnePack->AddField("stock_code", 'S', 6);
    lpOnePack->AddField("market_no", 'S', 4);
    lpOnePack->AddField("entrust_bs", 'C', 1);
    lpOnePack->AddField("entrust_oc", 'C', 1);
    lpOnePack->AddField("entrust_price_type", 'C', 1);
    lpOnePack->AddField("entrust_price", 'D');
    lpOnePack->AddField("entrust_amount", 'I');
    lpOnePack->AddField("valid_times", 'C', 1);
    lpOnePack->AddField("trade_flag", 'C', 1);
    lpOnePack->AddField("signal_id", 'I');
    lpOnePack->AddField("timestamp", 'I');

    lpOnePack->AddStr(GetSignalName().c_str());
    lpOnePack->AddStr(_SignalData.stock.c_str());
    lpOnePack->AddStr(_SignalData.market.c_str());
    lpOnePack->AddChar(_SignalData.bs);
    lpOnePack->AddChar(_SignalData.oc);
    lpOnePack->AddChar(_SignalData.pt);
    lpOnePack->AddDouble(_SignalData.price);
    lpOnePack->AddInt(_SignalData.volume);
    lpOnePack->AddChar(_SignalData.time);
    lpOnePack->AddChar(_SignalData.flag);
    lpOnePack->AddInt(_SignalData.signal_id);
    lpOnePack->AddInt(_SignalData.timestamp);
    lpOnePack->EndPack();

    IF2UnPacker* lpUnPack = lpOnePack->UnPack();

    //业务包构造完毕
    //调用业务的发送接口进行发布
    if (m_stT2.lpPublish) {
        int iRet = m_stT2.lpPublish->PubMsgByPacker((char*)m_stT2.strTopicName.c_str(), lpUnPack, 5000, NULL/*,true,&uRecordTime*/);
        if (iRet != 0) {
            std::cout << "T2消息发送失败, 内容为: {signal_name[" << GetSignalName() << "], stock_code[" << _SignalData.stock
                << "], market_no[" << _SignalData.market << "], entrust_bs[" << _SignalData.bs
                << "], entrust_oc[" << _SignalData.oc << "], signal_id[" << _SignalData.signal_id
                << "], timestamp[" << _SignalData.timestamp << "]}, 错误信息为：" << m_stT2.lpT2SrvConn->GetErrorMsg(iRet) << std::endl;
        }
        else if (IsVerbose()) {
            std::cout << "T2 sent " << m_stT2.lpT2SrvConn->GetErrorMsg(iRet) << ": {signal_name[" << GetSignalName() << "], stock_code[" << _SignalData.stock
                << "], market_no[" << _SignalData.market << "], entrust_bs[" << _SignalData.bs
                << "], entrust_oc[" << _SignalData.oc << "], signal_id[" << _SignalData.signal_id
                << "], timestamp[" << _SignalData.timestamp << "]}"<< std::endl;
        }
    }

    lpOnePack->FreeMem(lpOnePack->GetPackBuf());
    lpOnePack->Release();
    return;
}

bool operator<(const Channel& c1st, const Channel& c2nd) {
    return c1st.m_dbPriority < c2nd.m_dbPriority;
}


//以下各回调方法的实现仅仅为演示使用
unsigned long CCallbackImpl::QueryInterface(const char* iid, IKnown** ppv)
{
    return 0;
}

unsigned long CCallbackImpl::AddRef()
{
    return 0;
}

unsigned long CCallbackImpl::Release()
{
    return 0;
}
void CCallbackImpl::OnConnect(CConnectionInterface* lpConnection)
{
    puts("CCallbackImpl::OnConnect");
}

void CCallbackImpl::OnSafeConnect(CConnectionInterface* lpConnection)
{
    puts("CCallbackImpl::OnSafeConnect");
}

void CCallbackImpl::OnRegister(CConnectionInterface* lpConnection)
{
    puts("CCallbackImpl::OnRegister");
}

void CCallbackImpl::OnClose(CConnectionInterface* lpConnection)
{
    puts("CCallbackImpl::OnClose");
}

void CCallbackImpl::OnSent(CConnectionInterface* lpConnection, int hSend, void* reserved1, void* reserved2, int nQueuingData)
{

}

void CCallbackImpl::OnReceivedBiz(CConnectionInterface* lpConnection, int hSend, const void* lpUnpackerOrStr, int nResult)
{
    puts("CCallback::OnReceivedBiz");
}
void CCallbackImpl::OnReceivedBizEx(CConnectionInterface* lpConnection, int hSend, LPRET_DATA lpRetData, const void* lpUnpackerOrStr, int nResult)
{
    puts("CCallback::OnReceivedBizEx");
}
void CCallbackImpl::OnReceivedBizMsg(CConnectionInterface* lpConnection, int hSend, IBizMessage* lpMsg)
{
    puts("CCallback::OnReceivedBizMsg");
}
void CCallbackImpl::Reserved1(void* a, void* b, void* c, void* d)
{
}

void CCallbackImpl::Reserved2(void* a, void* b, void* c, void* d)
{
}
int  CCallbackImpl::Reserved3()
{
    return 0;
}

void CCallbackImpl::Reserved4()
{
}

void CCallbackImpl::Reserved5()
{
}

void CCallbackImpl::Reserved6()
{
}

void CCallbackImpl::Reserved7()
{
}

