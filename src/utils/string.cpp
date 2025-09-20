// dcapp includes
#include <utils/string.hpp>

// library includes

// c++ standard includes
#include <stdexcept>
#include <string>
#include <vector>

// convert a string to a double
double dc_utils_string_to_float(const std::string &text)
{
    double result = 0;
    try
    {
        result = std::stod(text);
    }
    catch (const std::invalid_argument)
    {
        result = (double)dc_utils_string_to_integer(text);
    }
    catch (const std::out_of_range)
    {
        result = (double)dc_utils_string_to_integer(text);
    }
    return result;
}

// convert a string to an integer
int dc_utils_string_to_integer(const std::string &text)
{
    int result = 0;
    try
    {
        result = std::stoi(text);
    }
    catch (const std::invalid_argument)
    {
        result = (int)dc_utils_string_to_boolean(text);
    }
    catch (const std::out_of_range)
    {
        result = (int)dc_utils_string_to_boolean(text);
    }
    return result;
}

// convert a string to a boolean
bool dc_utils_string_to_boolean(std::string text)
{
    text = dc_utils_trim_whitespace(text);
    if (text.empty())
    {
        return false;
    }

    // to lowercase
    for (char &c : text)
    {
        if (c >= 'A' && c <= 'Z')
        {
            c += 32;
        }
    }

    if (text == "false" || text == "no" || text == "off")
    {
        return false;
    }

    return true;
}

// compute an sdbm hash from a string
std::string dc_utils_string_to_hash(const std::string &text)
{
    uint64_t hash = 0;
    for (const char &c : text)
    {
        hash = c + (hash << 6) + (hash << 16) - hash;
    }
    std::string stringHash = std::to_string(hash);
    stringHash.insert(0, 20 - stringHash.size(), '0');
    return stringHash;
}

// trim whitespace from string
std::string dc_utils_trim_whitespace(std::string text)
{
    if (text.length() > 0)
    {
        static const std::string whitespace = " \t\n\v\f\r";

        // if no characters, use empty string
        std::size_t firstChar = text.find_first_not_of(whitespace);
        if (firstChar == std::string::npos)
        {
            return "";
        }

        // if whitespace occurs before first character, trim it
        std::size_t firstWhitespace = text.find_first_of(whitespace);
        if (firstWhitespace != std::string::npos && firstWhitespace < firstChar)
        {
            text = text.substr(firstChar, std::string::npos);
        }

        // if whitespace occurs after last character, trim it
        std::size_t lastWhitespace = text.find_last_of(whitespace);
        std::size_t lastChar = text.find_last_not_of(whitespace);
        if (lastWhitespace != std::string::npos && lastWhitespace > lastChar)
        {
            text = text.substr(0, lastChar + 1);
        }
    }

    return text;
}

// split string by delimiters provided
std::vector<std::string> dc_utils_split_string_by_delimiters(std::string input, const std::string &delimiters)
{
    std::vector<std::string> output;
    if (input.length() > 0)
    {
        std::size_t startIndex = 0;
        while (startIndex != std::string::npos)
        {

            // get indices of next space-delimited string
            std::size_t stringStartIndex = input.find_first_not_of(delimiters, startIndex);
            std::size_t stringEndIndex = input.find_first_of(delimiters, stringStartIndex);

            // if the string exists, get the substring
            if (stringStartIndex != std::string::npos)
            {
                size_t substringLength = stringEndIndex - stringStartIndex;
                if (stringEndIndex == std::string::npos)
                {
                    substringLength = std::string::npos;
                }
                output.push_back(input.substr(stringStartIndex, substringLength));
            }

            // update starting index
            startIndex = input.find_first_not_of(delimiters, stringEndIndex);
        }
    }
    return output;
}
