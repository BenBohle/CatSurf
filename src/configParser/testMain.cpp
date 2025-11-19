#include "../../include/configParser.hpp"
#define RED   "\033[31m"
#define BLUE  "\033[34m"
#define RESET "\033[0m"


void ConfigParser::test_print()
{
    std::cout << BLUE << "Global Config vals:" << std::endl;
    std::cout << RESET << "wp: " << global_config.worker_processes << " el: " << global_config.error_log << " pid: " << global_config.pid << std::endl;
    std::cout << RED << "------------------------------------------------" << std::endl;
    std::cout << BLUE <<"Server Config vals:" << std::endl;
    for (size_t i = 0; i < servers.size(); i++)
    {
        std::cout << BLUE << "server" << i << ": " << RESET << std::endl;
        std::cout << "listen: " << servers[i].listen_port << " root: " << servers[i].root << std::endl;
        std::cout << "index_files: ";
        for (size_t j = 0; j < servers[i].index_files.size(); j++)
            std::cout << servers[i].index_files[j] << ", ";
        std::cout << std::endl;
        std::cout << "server_name: ";
        for (size_t j = 0; j < servers[i].server_name.size(); j++)
            std::cout << servers[i].server_name[j] << ", ";
        std::cout << std::endl;
        std::cout << "error_pages: ";
        for (auto it = servers[i].error_pages.begin(); it != servers[i].error_pages.end(); it++)
            std::cout << it->first << " - " << it->second << ", ";
        std::cout << std::endl;
        std::cout << RED << "------------------------------------------------" << std::endl;
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "invalid amount of arguments: <programname + configfile>" << std::endl;
        return 1;
    }
    ConfigParser test;
    try
    {
        test.parse(argv[1]);
        test.test_print();
    }
    catch (const std::exception& e)
    {
        std::cerr << RED << "Configuration error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}