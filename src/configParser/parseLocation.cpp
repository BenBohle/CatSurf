#include "../../include/configParser.hpp"
#include <set>

void ConfigParser::setLocDirective(const std::string& key, const std::string& value, Type t, LocationConfig& loc)
{
    if (!validateType(t, value))
        throw std::runtime_error("Invalid value for directive: " + key + " inside Location Block");
    if (key == "root")
        loc.root = value;
    else if (key == "autoindex")
    try
    {
        loc.autoindex = stoi(value);
    }
    catch (std::exception& e)
    {
        throw std::runtime_error("Invalid type for directive autoindex" + value);
    }
}

void ConfigParser::setLocDirective(const std::string& key, const std::vector<std::string>& value, Type t, LocationConfig& loc)
{
    if (!validateType(t, value))
        throw std::runtime_error("Invalid value for directive: " + key);
    if (key == "index_files")
        loc.index_files = value;
    else if (key == "allow_methods")
        loc.allow_methods = value;
}

void ConfigParser::parseLocation(const std::vector<std::string>& tokens, size_t& i, ServerConfig& serv)
{
    i++;
    if (i >= tokens.size() || !isLocationPath(tokens[i]))
        throw std::runtime_error("invalid path for Location Block");

    LocationConfig loc;
    loc.path = tokens[i];
    i++;

    if (i >= tokens.size() || tokens[i] != "{")
        throw std::runtime_error("ConfigSyntaxError: expected '{' after 'server'");
    i++;

    std::set<std::string> duplicateCheck;

    while (i < tokens.size() && tokens[i] != "}")
    {
        const std::string& key = tokens[i]; 
        if (duplicateCheck.count(key) > 0)
            throw std::runtime_error("Duplicate directive: " + key);
        duplicateCheck.insert(key); 

        if (i + 1 >= tokens.size())
            throw std::runtime_error("Missing value for directive: " + key);
        else if (grammar.at(SERVER).find(key) != grammar.at(SERVER).end())
        {
            Type t = grammar.at(SERVER).at(key);

            if (t == FILENAME || t == METH)
            {
                std::vector<std::string> values;
                while (i < tokens.size() && tokens[i] != ";")
                {
                    values.push_back(tokens[i]);
                    i++;
                }
                if (i >= tokens.size() || tokens[i] != ";")
                    throw std::runtime_error("Syntax error: Missing ';'");
                setLocDirective(key, values, t, loc);
                i++;
            }
            else
            {
                const std::string& value = tokens[i + 1];
                if (i + 2 >= tokens.size() || tokens[i + 2] != ";")
                    throw std::runtime_error("Syntax error: Missing ';'");
                if (!validateType(t, value))
                    throw std::runtime_error("Invalid type for directive: " + key + " inside Location Block");
                setLocDirective(key, value, t, loc);
                i += 3;
            }
        }
        else
            throw std::runtime_error("Unknown directive in Location Block: " + key);
    }
     if (i >= tokens.size() || tokens[i] != "}")
        throw std::runtime_error("Unclosed location block");
    i++;
    //set location defaults?
    serv.locations.push_back(loc);
}
