// dcapp includes
#include <utils/file-utils.hpp>

// library includes

// c++ standard includes
#include <filesystem>

// TOOD dereference constants first
std::string filepathToCanonical(std::string file, const std::string &directory)
{
    // expand constants
    std::filesystem::path filePath = std::filesystem::path(file);
    if (!file.empty() && filePath.is_relative())
    {
        std::filesystem::path cleanedFilePath = std::filesystem::path(directory) / file;
        file = cleanedFilePath.string();
    }

    // convert to canonical
    return std::filesystem::weakly_canonical(file);
}
