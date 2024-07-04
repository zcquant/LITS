#ifndef __ZCONFINI__
#define __ZCONFINI__

#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <string>

#include "string.h"

using namespace std;

class ZConfIniMapMap
{
private:
    /* data */
public:
    ZConfIniMapMap();
    ~ZConfIniMapMap();

    map<string, map<string, string>> _conf_map;

    void InitFromFile(const char *);
    void PrintConfIni();

};

ZConfIniMapMap::ZConfIniMapMap()
{
}

ZConfIniMapMap::~ZConfIniMapMap()
{
}


#endif