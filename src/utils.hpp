#pragma once

#include <string>
#include <vector>

namespace utils {

std::string strip(const std::string& string,
                  const std::string& chars);

bool isString(const std::string& string,
              const std::string& value);

std::string lowercase(const std::string& string);

std::vector<std::string> split(const std::string& string,
                               char delimiter);

}