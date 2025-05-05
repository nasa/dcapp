// dcapp includes
#include <stdexcept>
#include <utils/file-utils.hpp>

// library includes
// TODO CLEAN THIS UP
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#elif defined(__APPLE__)
    #include <mach-o/dyld.h>
#elif defined(__linux__) || defined(__unix__)
#endif

// c++ standard includes
#include <filesystem>

std::string filepathToCanonical(std::string file, const std::string &directory)
{
    std::filesystem::path filePath = std::filesystem::path(file);
    if (!file.empty() && filePath.is_relative())
    {
        std::filesystem::path cleanedFilePath = std::filesystem::path(directory) / file;
        file = cleanedFilePath.string();
    }

    // convert to canonical
    return std::filesystem::weakly_canonical(file);
}

std::string getExeFilepath()
{
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
   // TODO implement
#elif defined(__APPLE__)
    char path[1024];
    uint32_t length;
    _NSGetExecutablePath(path, &length);
    return std::string(path);
#elif defined(__linux__) || defined(__unix__)
    return std::filesystem::canonical("/proc/self/exe").string();
#endif
    throw std::runtime_error("Unknown OS in file-utils.cpp");
}
