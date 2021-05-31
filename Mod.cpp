#include <iostream>

#include "json/single_include/nlohmann/json.hpp"
#include "EternalModLoader.hpp"

Mod::Mod(std::string name, std::string &json)
{
    nlohmann::json modJson = nlohmann::json::parse(json, nullptr, true, true);

    if (modJson.contains("name"))
        Name = modJson["name"].get<std::string>();

    if (modJson.contains("description"))
        Description = modJson["description"].get<std::string>();

    if (modJson.contains("version"))
        Version = modJson["version"].get<std::string>();

    if (modJson.contains("loadPriority"))
        LoadPriority = modJson["loadPriority"].get<int>();

    if (modJson.contains("requiredVersion"))
        RequiredVersion = modJson["requiredVersion"].get<int>();
}