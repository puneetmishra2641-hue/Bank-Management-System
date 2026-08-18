#ifndef DATABASE_CONFIG_H
#define DATABASE_CONFIG_H

#include <string>

struct DatabaseConfig
{
    std::string host;
    std::string user;
    std::string password;
    std::string dbName;
    unsigned int port;
};

DatabaseConfig loadDatabaseConfig();

#endif