#include "../../include/configParser.hpp"

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
    }
    catch (const std::exception& e)
    {
        std::cerr << "Configuration error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}