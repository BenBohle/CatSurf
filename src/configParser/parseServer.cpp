
#include "../../include/configParser.hpp"

/* root - defaults to some system path like /var/www/html
server_name - defaults to empty/catch-all
index - defaults to index.html
autoindex - defaults to off
client_max_body_size - defaults to 1m or similar
error_page - uses default error pages */

void setServerDefaults(ServerConfig &serv)
{
    if (serv.root.empty())
        serv.root = "/var/www/html";
    if (serv.server_name.empty())
        serv.server_name = {"_"}
    if (serv.index.empty())
        serv.index = {"index.html"};
    if (serv.error_page.empty())
        serv.error_page = {{404, "/404.html"}, {500, "/50x.html"}}
}

void ConfigParser::setServerDirective(const std::string& key, const std::string& value, Type t)
{
    if (!validateType(t, value))
        throw std::runtime_error("Invalid value for directive: " + key);
    if (key == "listen")
        servers[server_amnt].listen_port = stoi(value);
    else if (key == "root")
        servers[server_amnt].root = value;
}

void ConfigParser::setServerDirective(const std::string& key, const std::vector<string>& value, Type t, ServerConfig& serv)
{
    if (!validateType(t, value))
        throw std::runtime_error("Invalid value for directive: " + key);
    if (key == "index")
        serv.index = value;
    else if (key == "server_name")
        serv.server_name = value;
    else if (key == "error_page") // need to fix map thingy
    {
        const std::string& path = values.back();
        for (size_t i = 0; i < values.size() - 1; i++)
        {
            int code = std::stoi(values[i]);
            serv.error_page[code] = path;
        }
    }
}

void ConfigParser::parseServer(const std::vector<std::string>& tokens, size_t& i)
{
    ServerConfig serv;
    i++;
    if (i >= tokens.size() || tokens[i] != "{")
        throw std::runtime_error("ConfigSyntaxError: expected '{' after 'server'");
    i++;
    
    while (i < tokens.size() && tokens[i] != "}")
    {
        const std::string& key = tokens[i]; 
        if (i + 1 >= tokens.size())
            throw std::runtime_error("Missing value for directive: " + key);
        if (key == "location")
            parseLocation();
        else if (grammar.at(SERVER).find(key) != grammar.at(SERVER).end())
        {
            Type t = grammar.at(SERVER).at(key);

            else if (t == DOMAIN|| t == FILE || t == MAP)
            {
                std::vector<std::string> values;
                i++;
                while (i < tokens.size() && tokens[i] != ";")
                {
                    values.push_back(tokens[i]);
                    i++;
                }
                if (i >= tokens.size() || tokens[i] != ";")
                    throw std::runtime_error("Syntax error: Missing ';'");
                setServerDirective(server, key, values, t);
                i++;
            }
            else 
            {
                const std::string& value = tokens[i + 1];
                if (i + 2 >= tokens.size() || tokens[i + 2] != ";")
                    throw std::runtime_error("Syntax error: Missing ';'");
                if (!validateType(t, value))
                    throw std::runtime_error("Invalid type for directive: " + key);
                setServerDirective(server, key, value, t);
                i += 3;
            }
        }
        else
            throw std::runtime_error("Unknown directive in server block: " + key);
    }
    
    if (i >= tokens.size() || tokens[i] != "}")
        throw std::runtime_error("Unclosed server block");
    i++;
    if (server.listen_port == 0)
        throw std::runtime_error("Missing required 'listen' directive in server block");
    
    /* setServerDefaults(server); */
    
    servers.push_back(serv);
}
