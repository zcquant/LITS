#include <iostream>
#include <queue>
//#include "../include/mode.h"
#include "msgtool.h"
#include "tinyxml2.h"
#include "channel.h"

#ifdef USING_MSGTOOL_T2

IChannelMgr* g_lpChannelMgr = nullptr;

void SendT2Message(public_test& _SignalData)
{
    if (nullptr == g_lpChannelMgr)
    {
        g_lpChannelMgr = new ChannelManager();
        g_lpChannelMgr->Init();
    }

    g_lpChannelMgr->Publish(_SignalData);

    return;
}

#endif
