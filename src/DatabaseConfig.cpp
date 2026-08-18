#include "DatabaseConfig.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

DatabaseConfig loadDatabaseConfig()
{

    std::ifstream file(".env");

    if (!file.is_open())
    {
        throw std::runtime_error("Could not open .env file");
    }

    std::unordered_map<std::string, std::string> config;

    std::string line;

    while (std::getline(file, line))
    {

        // Ignore empty lines
        if (line.empty())
            continue;

        // Ignore comments
        if (line[0] == '#')
            continue;

        size_t pos = line.find('=');

        if (pos == std::string::npos)
            continue;

        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        config[key] = value;
    }

    DatabaseConfig dbConfig;

    dbConfig.host = config.at("DB_HOST");
    dbConfig.user = config.at("DB_USER");
    dbConfig.password = config.at("DB_PASSWORD");
    dbConfig.dbName = config.at("DB_NAME");
    dbConfig.port = std::stoi(config.at("DB_PORT"));

    return dbConfig;
}