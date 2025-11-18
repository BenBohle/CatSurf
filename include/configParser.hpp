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
    unsigned int worker_processes;
    std::string error_log;
    std::string pid;
};

struct LocationConfig
{
    std::string path;
    std::string root;
    bool autoindex;
    std::vector<std::string> index_files;
    std::vector<std::string> allowed_methods;
};

struct ServerConfig
{
    std::vector<std::string> server_name;
    int listen_port;
    std::string root;
    std::vector<std::string> index_files;
    std::map<int, std::string> error_pages;
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
    FILENAME,
    DOMAIN,
    MAP,
    BLOCK,
    METH
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
    int server_amnt;

    void parseGlobalConfig(const std::vector<std::string>& tokens, size_t& i);
    void setDefaultGC();
    void setServerDefaults(ServerConfig &serv);
    void setServerDirective(const std::string& key, const std::string& value, Type t, ServerConfig& serv);
    void setServerDirective(const std::string& key, const std::vector<std::string>& value, Type t, ServerConfig& serv);
    
    void setGlobalDirective(const std::string& key, const std::string& value);
    void parseServer(const std::vector<std::string>& tokens, size_t& i);
    void required();

    public:
    ConfigParser();
    ~ConfigParser();

    void parse(const std::string& path);
    const GlobalConfig& getGlobalConfig() const;
    const std::vector<ServerConfig>& getServers() const;
    void test_print();
};

/***********************************************************/
/*                  EXTERNAL FUNCTIONS                     */
/***********************************************************/


std::vector<std::string> tokenizeFile(const std::string& path);
bool validateType(Type t, const std::string& value);
bool validateType(Type t, const std::vector<std::string>& value);
bool isNumber(const std::string& str);
bool isPath(const std::string& str);
bool isBoolean(const std::string& str);
bool isFilename(const std::string& str);
bool isDomainname(const std::string& str);
bool isMethod(const std::string& str);

#endif