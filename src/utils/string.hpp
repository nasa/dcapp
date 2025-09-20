#pragma once

// dcapp includes

// library includes

// c++ standard includes
#include <string>
#include <vector>

double dc_utils_string_to_float(const std::string &text);
int    dc_utils_string_to_integer(const std::string &text);
bool   dc_utils_string_to_boolean(std::string text);

std::string              dc_utils_string_to_hash(const std::string &text);
std::string              dc_utils_trim_whitespace(std::string text);
std::vector<std::string> dc_utils_split_string_by_delimiters(std::string input, const std::string &delimiters);

const std::string dc_utils_string_whitespace = " \t\n\v\f\r";
