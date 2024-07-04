#include "zconfini.h"

using namespace std;

void ZConfIniMapMap::InitFromFile(const char *IniFile)
{
    ifstream ini_file(IniFile, ifstream::in);
    char x[256];
    string *mp;
    while (ini_file.getline(x, 256))
    {
        if (strlen(x) == 0)
            continue;
        if (x[0] == '[')
        {
            x[strlen(x) - 1] = '\0';
            string mk(x + 1);
            map<string, string> mvm;
            _conf_map[mk] = mvm;
            mp = &mk;
        }
        else
        {
            string kv(x);
            istringstream ss(kv);
            string mvk, mvv;
            getline(ss, mvk, '=');
            getline(ss, mvv, '=');
            if (!(mp->empty()))
            {
                _conf_map[*mp][mvk] = mvv;
            }
        }
    }
    ini_file.close();
}

void ZConfIniMapMap::PrintConfIni()
{
    for (auto i = _conf_map.begin(); i != _conf_map.end(); ++i)
    {
        cout << "The Map " << i->first << endl;
        for (auto j = i->second.begin(); j != i->second.end(); ++j)
        {
            cout << "The Key " << j->first << " The Value " << j->second << endl;
        }
    }
}