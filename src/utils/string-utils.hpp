#pragma once

// dcapp includes

// library includes

// c++ standard includes
#include <string>
#include <vector>

double stringToFloat(const std::string &text);
int stringToInteger(const std::string &text);
bool stringToBoolean(std::string text);

std::string stringToHash(const std::string &text);
std::string trimWhitespace(std::string text);
std::vector<std::string> splitStringByDelimiters(std::string input, const std::string &delimiters);

const std::string stringWhitespace = " \t\n\v\f\r";
