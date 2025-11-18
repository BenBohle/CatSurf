#include "../../include/configParser.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
// parse config: look for file extension - order: global (or missing) - server must have listen, server_name; root & error & location optional { location{}}

ConfigParser::ConfigParser() {}

ConfigParser::~ConfigParser() {}

bool validateType(Type t, const std::vector<std::string>& value)
{
    if (value.empty())
        return false;
    switch(t)
    {
        case METH:
            for (size_t i = 0; i < value.size(); i++)
            {
                if (!isMethod(value[i]))
                    return false;
            }
                return true;
        case DOMAIN:
            for (size_t i = 0; i < value.size(); i++)
            {
                if (!isDomainname(value[i]))
                    return false;
            }
                return true;
        case FILENAME:
            for (size_t i = 0; i < value.size(); i++)
            {
                if (!isFilename(value[i]))
                    return false;
            }
                return true;
        case MAP:
            if (value.size() < 2)
                return false;
            for (size_t i = 0; i < value.size() - 1; i++)
            {
                if (!isNumber(value[i]))
                    return false;
            }
            if (!isPath(value.back()))
                return false;
            return true;
        default:
            return false;
    }
}

bool validateType(Type t, const std::string& value)
{
    switch (t) 
    {
        case NUMBER:
            return isNumber(value);
        case NBR_AUTO:
            return isNumber(value) || (value == "auto");
        case PATH:
            return isPath(value);
        case BOOLEAN:
            return isBoolean(value);
        default:
            return false;
    }
}

std::vector<std::string> tokenizeFile(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
        throw std::runtime_error("Cannot open file: " + path);
    
    std::vector<std::string> tokens;
    std::string line;

    while (std::getline(file,line))
    {
        size_t comment = line.find('#');
        if (comment != std::string::npos)
            line = line.substr(0, comment);

        std::istringstream iss(line);
        std::string word;

        while (iss >> word)
        {
            if (!word.empty() && word.back() == ';')
            {
                tokens.push_back(word.substr(0, word.size() - 1));
                tokens.push_back(";");
            }
            else
                tokens.push_back(word);
        }
    }
    return tokens;
}

void ConfigParser::parse(const std::string& path)
{
    std::vector<std::string> tokens = tokenizeFile(path);

    size_t i = 0;

    parseGlobalConfig(tokens, i);
    std::cout << "Global Config vals: wp: " << global_config.worker_processes << " el: " << global_config.error_log << " pid: " << global_config.pid << std::endl;

    while (i < tokens.size()) 
    {
        if (tokens[i] == "server")
            parseServer(tokens, i);
        else
            throw std::runtime_error("Unexpected token outside server block: " + tokens[i]);
    }
    if (servers.empty())
        throw std::runtime_error("No server blocks found in config file");
}

const GlobalConfig& ConfigParser::getGlobalConfig() const
{
    return global_config;
}

const std::vector<ServerConfig>& ConfigParser::getServers() const
{
    return servers;
}