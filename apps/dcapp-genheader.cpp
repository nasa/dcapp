#include <dcapp-data.hpp>
#include <utils/xml-utils.hpp>
#include <utils/string-utils.hpp>

#include <cstdio>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

int main(int argc, char **argv)
{
    std::string configRelativePath = "/data/nreagan/dcapp-vk/samples/test/test.xml";

    // set paths
    std::filesystem::path fsFilePath = std::filesystem::canonical(configRelativePath);
    std::filesystem::path fsDirPath = fsFilePath.parent_path();
    std::string configFilePath = fsFilePath.string();
    std::string configDirPath = fsDirPath.string();

    // create cache and log dirs
    std::filesystem::path fsExePath = std::filesystem::canonical("/proc/self/exe");
    std::filesystem::path fsLogPath = fsExePath.parent_path() / ".." / "logs";
    std::filesystem::create_directory(fsLogPath);
    std::string logDirPath = std::filesystem::canonical(fsLogPath).string();
    std::filesystem::path fsCachePath = fsExePath.parent_path() / ".." / "cache";
    std::filesystem::create_directory(fsCachePath);
    std::string cacheDirPath = std::filesystem::canonical(fsCachePath).string();

    // begin setting up dcappData object
    dc::dcData.configFilePath = configFilePath;
    dc::dcData.configDirPath = configDirPath;
    dc::dcData.logDirPath = logDirPath;
    dc::dcData.cacheDirPath = cacheDirPath;

    // set environment (used for dcapp XMLs)
    setenv("dcappDisplayHome", dc::dcData.configDirPath.c_str(), 1);

    // load XML file
    int xmlResult = xmlCheckFilename(configFilePath.c_str());
    if (xmlResult == 0)
    {
        throw std::runtime_error("Failed to stat configuration file: " + configFilePath);
    }
    else if (xmlResult == 2)
    {
        throw std::runtime_error("Configuration file is a directory: " + configFilePath);
    }
    dc::dcData.doc = xmlReadFile(configFilePath.c_str(), "UTF-8", XML_PARSE_NOBLANKS);
    if (!dc::dcData.doc)
    {
        throw std::runtime_error("Unable to read configuration file: " + configFilePath);
    }

    // clean XML file
    dc::cleanXmlData();
}
