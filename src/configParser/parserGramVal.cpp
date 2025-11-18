#include "../../include/configParser.hpp"
#include <regex>

// what inside locations?
// default for anything besides listen
// client_max_body_size?

const std::map<Block, std::map<std::string, Type>> ConfigParser::grammar =
{
    {GLOBAL, 
        {
            {"worker_processes", NBR_AUTO},
            {"error_log", PATH},
            {"pid", PATH}
        }
    },
    {SERVER, 
        {
            {"listen", NUMBER},
            {"root", PATH},
            {"index", FILE},
            {"server_name", DOMAIN},
            {"error_page", MAP},
            {"location", BLOCK}
        }
    },
    {LOCATION, 
        {
            {"root", PATH},
            {"autoindex", BOOLEAN},
            {"index", FILE},
            {"methods", METH}
    }
    }
};

/* void ConfigParser::required() 
{
    if (!has("listen"))
        throw std::runtime_error("Missing required 'listen' directive");
} */

bool isMethod(const std::string& str)
{
    return str == "GET" || str == "POST" || str == "DELETE" || 
           str == "PUT" || str == "HEAD" || str == "OPTIONS";
}

bool isFilename(const std::string& str)
{
    if (str.empty() || str.find_first_of("/\0") != std::string::npos || str.length() >= 255)
        return false;
    return true;
}

bool isDomainname(const std::string& str)
{
    std::regex pat("^[A-Za-z0-9-]{1,63}\\.[A-Za-z]{2,6}$");
    return regex_match(str, pat);
}

bool isNumber(const std::string& str)
{
    if (str.empty())
        return false;
    return str.find_first_not_of("0123456789") == std::string::npos;
}

bool isPath(const std::string& str)
{
    return !str.empty() && (str[0] == '/' || str[0] == '.');
}

bool isBoolean(const std::string& str)
{
    return str == "on" || str == "off";
}