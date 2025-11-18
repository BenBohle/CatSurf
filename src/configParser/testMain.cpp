#include "../../include/configParser.hpp"

void ConfigParser::test_print()
{
    std::cout << "Global Config vals:" << std::endl;
    std::cout << "wp: " << global_config.worker_processes << " el: " << global_config.error_log << " pid: " << global_config.pid << std::endl;

    std::cout << "Server Config vals:" << std::endl;
    for (size_t i = 0; i < servers.size(); i++)
    {
        std::cout << "server" << i << ": " << std::endl;
        std::cout << "listen: " << servers[i].listen_port << " root: " << servers[i].root << std::endl;
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
        std::cerr << "Configuration error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}