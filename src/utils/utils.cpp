#include "../../include/utils.hpp"
#include <cctype>
#include <cstdlib>

bool isNumber(const std::string& str)
{
    if (str.empty())
        return false;
    return (str.find_first_not_of("0123456789") == std::string::npos);
}

bool isListen(const std::string& str)
{
    size_t colon = str.find(':');
    
    if (colon != std::string::npos)
    {
        std::string ip = str.substr(0, colon);
        std::string port = str.substr(colon + 1);
        
        return isValidIP(ip) && isPort(port);
    } 
    else
        return isPort(str);
}

bool isValidIP(const std::string& ip)
{
    if (ip.empty())
        return false;
    
    int dots = 0;
    for (char c : ip)
    {
        if (c == '.')
            dots++;
        else if (!std::isdigit(c))
            return false;
    }
    return dots == 3;
}

bool isPort(const std::string& str)
{
    if (!isNumber(str))
        return false;
    try 
    {
        int a = std::stoi(str);
        return a > 1 && a < 65535;
    }
    catch (const std::exception& e)
    {
        return false;
    }
}

bool isMethod(const std::string& str)
{
    if (str.empty())
        return false;

    return str == "GET" || str == "POST" || str == "DELETE";
}

bool isDomainname(const std::string& str)
{
    if (str.empty() || str.length() > 253 || str.find('/') != std::string::npos)
        return false;

    for (char c : str)
    {
        if (!std::isalnum(c) && c != '.' && c != '-'/*  && c != ':' && c != '_' */)
            return false;
    }
    if (str[0] == '.' || str[0] == '-' || str.back() == '.' || str.back() == '-')
        return false;
    return true;
}

bool isValidIPv6(const std::string& ip)
{
    if (ip.empty())
        return false;

    int colonCount = 0;
    int doubleColonCount = 0;
    bool lastWasColon = false;

    for (size_t i = 0; i < ip.size(); ++i)
    {
        char c = ip[i];
        if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))
        {
            lastWasColon = false;
        }
        else if (c == ':')
        {
            if (lastWasColon)
            {
                doubleColonCount++;
                if (doubleColonCount > 1)
                    return false; // only one '::' allowed
            }
            colonCount++;
            lastWasColon = true;
        }
        else
        {
            return false; // invalid character
        }
    }

    return colonCount > 0; // must have at least one colon
}

bool isIPv6Host(const std::string& host)
{
    // Must start with '['
    if (host.empty() || host.front() != '[')
        return false;

    // Find closing bracket
    size_t closeBracket = host.find(']');
    if (closeBracket == std::string::npos)
        return false;

    std::string ip = host.substr(1, closeBracket - 1); // strip brackets
    if (!isValidIPv6(ip))
        return false;

    // Check optional port after closing bracket
    if (closeBracket + 1 == host.length())
        return true; // no port, valid
    else if (host[closeBracket + 1] == ':')
    {
        std::string portStr = host.substr(closeBracket + 2);
        return isPort(portStr);
    }

    return false; // invalid character after closing bracket
}
