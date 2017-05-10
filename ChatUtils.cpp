#include <ChatUtils.h>

using namespace std;

string
ChatUtils::trim(const string& s)
{
	static const string delims = "\t\r\n ";
	string::size_type last = s.find_last_not_of(delims);
	if(last == string::npos)
	{
		return string();
	}
    return s.substr(s.find_first_not_of(delims), last+1);
}