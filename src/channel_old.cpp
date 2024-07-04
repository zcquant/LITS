#include <iostream>
#include <algorithm>
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

    std::sort(m_vcChannel.begin(), m_vcChannel.end());

    cout << "Channel manager initialized" << endl;
    ToString();

    return 0;
}

int ChannelManager::Publish(public_test& _SignalData)
{
    for (const auto & c: m_vcChannel) {
        if (!c.MatchMarket(_SignalData.market)) {
            //cout << "Match none, market: "<< _SignalData.market << ", channel: " << c.GetID() << endl;
            continue;
        }
        c.SendT2Message(_SignalData);

        //cout << "Publish, market: " << _SignalData.market << ", channel: " << c.GetID() << endl;
    }
    return 0;
}

void ChannelManager::ToString() const
{
    for (const auto& c : m_vcChannel) {
        c.ToString();
    }
}

int ChannelManager::LoadConf()
{
    string strFile{ CLOUD_SIGNAL };
    XMLDocument cfg;
    XMLError err = cfg.LoadFile(strFile.c_str());
    if (err != XML_SUCCESS) {
        cerr << "配置文件:" << strFile.c_str() << ", 不存在或格式错误,请检查后重试" << endl;
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

void Channel::InitT2(const char* lp)
{
    m_stT2.strT2Conf = lp;
    string& sTopicName = m_stT2.strTopicName;
    CPublishInterface*& lpPublish = m_stT2.lpPublish;
    CConnectionInterface*& lpT2SrvConn = m_stT2.lpT2SrvConn;
    if (lpT2SrvConn == nullptr)
    {
        CConfigInterface* pConfig = nullptr;
        if (!GetT2Config(pConfig, m_stT2.strT2Conf.c_str()))
        {
            std::cout << "读取配置文件" << m_stT2.strT2Conf << "失败，请检查！" << std::endl;
            return;
        }

        if (!GetT2Connect(lpT2SrvConn, pConfig, nullptr))
        {
            std::cout << "创建T2 连接失败！" << std::endl;
            return;
        }

        std::cout << "创建T2 连接成功！" << std::endl;

        if (lpPublish == nullptr)
        {
            char* clientName = (char*)pConfig->GetString("mc", "client_name", "xtech");//获取发布的主题名
            sTopicName = pConfig->GetString("publish", "topic_name", "stp.jygt.signalpush");
            lpPublish = lpT2SrvConn->NewPublisher(clientName, 200, 5000);
            if (lpPublish == nullptr)
            {
                std::cout << "创建T2消息发布者失败，失败原因[" << lpT2SrvConn->GetMCLastError() << "]" << std::endl;
                pConfig->Release();
                lpT2SrvConn->Release();
                lpT2SrvConn = nullptr;
                return;
            }

            lpPublish->AddRef();
            std::cout << "创建T2消息发布者成功!" << std::endl;
        }
    }
}

string Channel::GetID() const
{
    return m_strID;
}

bool Channel::Authority() const
{
    cout << "Authority passed, channel: " << m_strID << endl;
    return true;
}

bool Channel::MatchMarket(const string& market) const
{
    return m_usetMarket.find(market) != m_usetMarket.end();
}

void Channel::SetID(const char* lp)
{
    m_strID = lp;
}

void Channel::SetPriority(double db)
{
    m_dbPriority = db;
}

void Channel::SetMarket(const char* lp)
{
    size_t pos = 0, found = 0;
    string str(lp);
    while (found != string::npos) {
        found = str.find("|", pos);
        m_usetMarket.insert(std::string(str, pos, found - pos));
        pos = found + 1;
    }
}

bool Channel::GetT2Config(CConfigInterface*& pConfig, std::string sConfigName)
{
    pConfig = NewConfig();
    if (pConfig == nullptr)
    {
        std::cout << "NewConfig Memory Allocate Failed";
        return false;
    }

    pConfig->AddRef();
    if (pConfig->Load(sConfigName.c_str()) != 0)
    {
        std::cout << "Load Configure File Failed, ConfigPath[" << sConfigName << "]";

        pConfig->Release();
        pConfig = nullptr;
        return false;
    }

    return true;
}

bool Channel::GetT2Connect(CConnectionInterface*& pConnection, CConfigInterface*& pConfig, CCallbackInterface* pCallback)
{
    pConnection = NewConnection(pConfig);
    if (pConnection == nullptr)
    {
        std::cout << "NewConnection Memory Allocate Failed";
        pConfig->Release();
        pConfig = nullptr;
        return false;
    }

    int ret = 0;
    pConnection->AddRef();
    //创建自定义类CCallback的对象（在创建连接时需传递此对象，请看下面代码）     

    //初始化连接对象，返回0表示初始化成功，注意此时并没开始连接服务器
    if (0 == (ret = pConnection->Create2BizMsg(pCallback)))
    {
        //正式开始连接，参数CONNECT_TIME_OUT为超时参数，单位是ms
        if (0 != (ret = pConnection->Connect(3000)))
        {
            std::cout << "T2 连接超时，错误信息:" << pConnection->GetErrorMsg(ret);

            pConfig->Release();
            pConfig = nullptr;
            pConnection->Release();
            pConnection = nullptr;

            return false;
        }
    }
    else
    {
        std::cout << "T2 连接失败，错误信息:" << pConnection->GetErrorMsg(ret);

        pConfig->Release();
        pConfig = nullptr;
        pConnection->Release();
        pConnection = nullptr;

        return false;
    }

    return true;
}

bool Channel::LoadConf(const void* lp)
{
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

    temp = element->FirstChildElement("t2_config");
    if (temp == nullptr) {
        cerr << "t2_config必须存在" << endl;
        return false;
    }
    else {
        InitT2(temp->GetText());
    }

    cout << "Channel loaded config, id: " << m_strID << endl;

    return true;
}

void Channel::ToString() const
{
    cout << "m_strID: " << m_strID << endl;
    cout << "m_dbPriority: " << m_dbPriority << endl;
    for (const auto& m: m_usetMarket) {
        cout << "m_usetMarket: " << m << endl;
    }
    cout << "m_stT2.strT2Conf: " << m_stT2.strT2Conf << endl;
    cout << "m_stT2.strTopicName: " << m_stT2.strTopicName << endl;
    cout << "m_stT2.lpPublish: " << m_stT2.lpPublish << endl;
    cout << "m_stT2.lpT2SrvConn: " << m_stT2.lpT2SrvConn << endl;
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

    lpOnePack->AddStr("xt100000");
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
        if (iRet != 0)
        {
            std::cout << "T2消息发送失败，内容为：{signal_name[xt100000], stock_code[" << _SignalData.stock
                << "], market_no[" << _SignalData.market << "], entrust_bs[" << _SignalData.bs
                << "], entrust_oc[" << _SignalData.oc << "], signal_id[" << _SignalData.signal_id
                << "], timestamp[" << _SignalData.timestamp << "]}, 错误信息为：" << m_stT2.lpT2SrvConn->GetErrorMsg(iRet) << std::endl;
        }
    }

    lpOnePack->FreeMem(lpOnePack->GetPackBuf());
    lpOnePack->Release();
    return;
}

bool operator<(const Channel& c1st, const Channel& c2nd)
{
    return c1st.m_dbPriority > c2nd.m_dbPriority;
}

