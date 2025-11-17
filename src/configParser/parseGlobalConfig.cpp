#include "../../include/configParser.hpp"
#include <thread>

void ConfigParser::setDefaultGC()
{
    if (global_config.worker_processes == 0)
        global_config.worker_processes = std::thread::hardware_concurrency();
    if (global_config.error_log.empty())
        global_config.error_log = "./logs/error.log";
    if (global_config.pid.empty())
        global_config.pid = "./webserv.pid";
}

void ConfigParser::setGlobalDirective(const std::string& key, const std::string& value, Type t)
{
    if (key == "worker_processes" && t == NBR_AUTO)
    {
        if (value == "auto")
            global_config.worker_processes = std::thread::hardware_concurrency();
        else
            global_config.worker_processes = std::stoi(value);
    }
    else if (key == "error_log" && t == PATH)
        global_config.error_log = value;
    else if (key == "pid" && t == PATH)
        global_config.pid = value;
    else 
        throw std::runtime_error("Unknown global directive: " + key);
}

void ConfigParser::parseGlobalConfig(const std::vector<std::string>& tokens, size_t& i)
{
    while (i < tokens.size() && tokens[i] != "server") 
    {
        const std::string& key = tokens[i];

        if (i + 1 >= tokens.size())
            throw std::runtime_error("Missing value for global directive: " + key);
        if (i + 2 >= tokens.size() || tokens[i + 2] != ";")
            throw std::runtime_error("Syntax error: Missing ';'");

        const std::string& value = tokens[i + 1];

        if (grammar.at(GLOBAL).find(key) != grammar.at(GLOBAL).end()) 
        {
            Type t = grammar.at(GLOBAL).at(key);
            if (!validateType(t, value))
                throw std::runtime_error("Invalid type for directive: " + key);

            setGlobalDirective(key, value, t);
        }
        else
            throw std::runtime_error("Invalid directive in global block: " + tokens[i]);
        i += 3;
    }
    setDefaultGC();
    if (i >= tokens.size())
        throw std::runtime_error("No server block found in config file");
}