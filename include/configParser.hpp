#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include <string>
#include <map>
#include <vector>
#include <iostream>

/***********************************************************/
/*                       STRUCTS                           */
/***********************************************************/

struct GlobalConfig
{
    int worker_processes;
    std::string error_log;
    std::string pid;
};

struct LocationConfig
{
    std::string path;
    std::string root;
    bool allow_upload;
};

struct ServerConfig
{
    std::string server_name;
    int listen_port;
    std::string root;
    std::vector<LocationConfig> locations;
};

struct Config
{
    GlobalConfig global;
    std::vector<ServerConfig> servers;
};

/***********************************************************/
/*                       ENUM                              */
/***********************************************************/

enum Type
{
    NUMBER,
    NBR_AUTO,
    PATH,
    BOOLEAN,
    STRING,
    LIST,
    MAP
};

enum Block
{
    GLOBAL,
    SERVER,
    LOCATION
};

/***********************************************************/
/*                       CLASS                             */
/***********************************************************/

class ConfigParser
{
    private:
    static const std::map<Block, std::map<std::string, Type>> grammar;

    GlobalConfig global_config;
    std::vector<ServerConfig> servers;

    void parseGlobalConfig(const std::vector<std::string>& tokens, size_t& i);
    void setDefaultGC();
    void setGlobalDirective(const std::string& key, const std::string& value, Type t);
    void parseServer(std::vector<std::string> tokens, size_t& i);
    void required();

    public:
    ConfigParser();
    ~ConfigParser();

    void parse(const std::string& path);
    const GlobalConfig& getGlobalConfig() const;
    const std::vector<ServerConfig>& getServers() const;
};

/***********************************************************/
/*                  EXTERNAL FUNCTIONS                     */
/***********************************************************/


std::vector<std::string> tokenizeFile(const std::string& path);
bool validateType(Type t, std::string value);
bool isNumber(const std::string& str);
bool isPath(const std::string& str);
bool isBoolean(const std::string& str);

#endif