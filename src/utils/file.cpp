// dcapp includes
#include <stdexcept>
#include <utils/file.hpp>

// library includes
// TODO CLEAN THIS UP
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#elif defined(__linux__) || defined(__unix__)
#endif

// c++ standard includes
#include <filesystem>

std::string dc_utils_filepath_to_canonical(std::string file, const std::string &directory) {
    std::filesystem::path file_path = std::filesystem::path(file);
    if (!file.empty() && file_path.is_relative()) {
        std::filesystem::path cleaned_file_path = std::filesystem::path(directory) / file;
        file                                    = cleaned_file_path.string();
    }

    // convert to canonical
    return std::filesystem::weakly_canonical(file);
}

std::string dc_utils_get_exe_filepath() {
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    // TODO implement
#elif defined(__APPLE__)
    char     path[1024];
    uint32_t length;
    _NSGetExecutablePath(path, &length);
    return std::string(path);
#elif defined(__linux__) || defined(__unix__)
    return std::filesystem::canonical("/proc/self/exe").string();
#endif
    throw std::runtime_error("Unknown OS in file-utils.cpp");
}
