// dcapp includes
#include <utils/string.hpp>

// library includes

// c++ standard includes
#include <string>
#include <vector>

static bool _string_is_float(const std::string &text) {
    char* end = nullptr;
    errno = 0;
    std::strtof(text.c_str(), &end);
    return (end != text.c_str() && *end == '\0' && errno == 0);
}

static bool _string_is_int(const std::string& text) {
    char* end = nullptr;
    errno = 0;
    std::strtol(text.c_str(), &end, 10);
    return (end != text.c_str() && *end == '\0' && errno == 0);
}

// convert a string to a double
double dc_utils_string_to_float(const std::string &text) {
    if (_string_is_float(text)) {
        return std::stod(text);
    } else {
        return (double)dc_utils_string_to_boolean(text);
    }
}

// convert a string to an integer
int dc_utils_string_to_integer(const std::string &text) {
    if (_string_is_int(text)) {
        return std::stoi(text);
    } else if (_string_is_float(text)){
        return (int)std::stod(text);
    } else {
        return (bool)dc_utils_string_to_boolean(text);
    }
}

// convert a string to a boolean
bool dc_utils_string_to_boolean(std::string text) {
    text = dc_utils_trim_whitespace(text);
    if (text.empty()) {
        return false;
    }

    // to lowercase
    for (char &c : text) {
        if (c >= 'A' && c <= 'Z') {
            c += 32;
        }
    }

    if (text == "false" || text == "no" || text == "off") {
        return false;
    }

    return true;
}

// compute an sdbm hash from a string
std::string dc_utils_string_to_hash(const std::string &text) {
    uint64_t hash = 0;
    for (const char &c : text) {
        hash = c + (hash << 6) + (hash << 16) - hash;
    }
    std::string string_hash = std::to_string(hash);
    string_hash.insert(0, 20 - string_hash.size(), '0');
    return string_hash;
}

// trim whitespace from string
std::string dc_utils_trim_whitespace(std::string text) {
    if (text.length() > 0) {
        static const std::string whitespace = " \t\n\v\f\r";

        // if no characters, use empty string
        std::size_t first_char = text.find_first_not_of(whitespace);
        if (first_char == std::string::npos) {
            return "";
        }

        // if whitespace occurs before first character, trim it
        std::size_t first_whitespace = text.find_first_of(whitespace);
        if (first_whitespace != std::string::npos && first_whitespace < first_char) {
            text = text.substr(first_char, std::string::npos);
        }

        // if whitespace occurs after last character, trim it
        std::size_t last_whitespace = text.find_last_of(whitespace);
        std::size_t last_char       = text.find_last_not_of(whitespace);
        if (last_whitespace != std::string::npos && last_whitespace > last_char) {
            text = text.substr(0, last_char + 1);
        }
    }

    return text;
}

// split string by delimiters provided
std::vector<std::string> dc_utils_split_string_by_delimiters(std::string input, const std::string &delimiters) {
    std::vector<std::string> output;
    if (input.length() > 0) {
        std::size_t start_index = 0;
        while (start_index != std::string::npos) {

            // get indices of next space-delimited string
            std::size_t string_start_index = input.find_first_not_of(delimiters, start_index);
            std::size_t string_end_index   = input.find_first_of(delimiters, string_start_index);

            // if the string exists, get the substring
            if (string_start_index != std::string::npos) {
                size_t substring_length = string_end_index - string_start_index;
                if (string_end_index == std::string::npos) {
                    substring_length = std::string::npos;
                }
                output.push_back(input.substr(string_start_index, substring_length));
            }

            // update starting index
            start_index = input.find_first_not_of(delimiters, string_end_index);
        }
    }
    return output;
}
