
#include "strutil.h"

namespace log
{
using namespace std;
vector<string> split(const string& src, const string& det, unsigned int count)
{
    vector<string> ret;
    string::size_type pos = 0;
    string::size_type use;
    string::size_type add = det.length();
    use = src.find(det);
    while(use != string::npos)
    {
        if((count != 0) && (count == (ret.size() + 1)))
        {
            break;
        }
        ret.push_back(src.substr(pos, use - pos));
        pos = use + add;
        use = src.find(det, pos);
    }
    ret.push_back(src.substr(pos));
    return ret;
}

vector<string> split(const string& src, char det, unsigned int count)
{
    vector<string> ret;
    string::size_type pos = 0;
    string::size_type use;
    use = src.find(det);
    while(use != string::npos)
    {
        if((count != 0) && (count == (ret.size() + 1)))
        {
            break;
        }
        ret.push_back(src.substr(pos, use - pos));
        pos = use + 1;
        use = src.find(det, pos);
    }
    ret.push_back(src.substr(pos));
    return ret;
}
}

