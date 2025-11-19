#include "../../include/configParser.hpp"

// what inside locations?
// default for anything besides listen
// client_max_body_size?

const std::map<Block, std::map<std::string, Type>> ConfigParser::grammar =
{
    {GLOBAL, 
        {
            {"worker_processes", WORK_PRC},
            {"error_log", PATH},
            {"pid", PATH}
        }
    },
    {SERVER, 
        {
            {"listen", PORT},
            {"root", PATH},
            {"index", FILENAME},
            {"server_name", DOMAIN},
            {"error_page", MAP},
            {"location", BLOCK}
        }
    },
    {LOCATION, 
        {
            {"root", PATH},
            {"autoindex", BOOLEAN},
            {"index", FILENAME},
            {"allow_methods", METH}
        }
    }
};

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
        if (!std::isalnum(c) && c != '.' && c != '-' && c != ':' && c != '_')
            return false;
    }
    if (str[0] == '.' || str[0] == '-' || str.back() == '.' || str.back() == '-')
        return false;
    return true;
}

bool isFilename(const std::string& str)
{
    if (str.empty() || str.find_first_of("/\0") != std::string::npos
        || str.length() > 255)
        return false;
    return true;
}

bool isNumber(const std::string& str)
{
    if (str.empty())
        return false;
    return (str.find_first_not_of("0123456789") == std::string::npos);
}

bool isErrorCode(const std::string& str)
{
    if (!isNumber(str))
        return false;
    try
    {
        int code = std::stoi(str);
        return code >= 400 && code <= 599;
    }
    catch (const std::exception &e)
    {
        return false;
    }
}

bool isWorkerProcesses(const std::string& str)
{
    if (!str.empty() && str == "auto")
        return true;
    if (!isNumber(str))
        return false;
    try
    {
        int workers = std::stoi(str);
        return (workers >= 0 && workers <= 1024);
    }
    catch (const std::exception&)
    {
        return false;
    }
}

bool isPort(const std::string& str)
{
    if (!isNumber(str))
        return false;
    try 
    {
        int a = stoi(str);
        return a > 1 && a < 65535;
    }
    catch (const std::exception& e)
    {
        return false;
    }
}

bool isPath(const std::string& str)
{
    if (str.empty())
        return false;
    if (str[0] == '/')
        return str.length() > 1;
    if (str.length() >= 2 && str[0] == '.' && str[1] == '/')
        return str.length() > 2;
    if (str.length() >= 3 && str[0] == '.' && str[1] == '.' && str[2] == '/')
        return str.length() > 3;
    return false;
}

bool isLocationPath(const std::string& str)
{
    if (str.empty() || str[0] != '/') 
    return false;

    if (str.find("..") != std::string::npos)
        return false;
    for (char c : str)
    {
        if (!std::isalnum(c) && c != '/' &&
            c != '-' && c != '.' && c != '_')
            return false;
    }
    return true;
}

bool isBoolean(const std::string& str)
{
    return str == "on" || str == "off";
}
