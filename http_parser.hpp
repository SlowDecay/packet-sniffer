#include <string>
#include <vector>
#include <sstream>

using namespace std;

struct http_parser
{
    string s;
    vector<string> lines;

    http_parser(const char* s)
    {
        this->s = s;
        stringstream ss(this->s);
        string line;

        while(getline(ss, line))
        {
            lines.push_back(line);
        }
    }

    bool isHTTP()
    {
        if(lines.empty()) return false;
        if(lines[0].substr(0, 4) == "HTTP") return false;
        return lines[0].find("HTTP") != string::npos;
    }

    string getReqOrRes()
    {
        if(lines.empty()) return "";
        return lines[0];
    }

    string getData()
    {
        if(lines.empty()) return "";
        if(lines[0].find("No Content") != string::npos) return "";
        return lines.back();
    }
};