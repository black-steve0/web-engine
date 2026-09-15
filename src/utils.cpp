#include <string>
#include <cctype>
#include <vector>
#include <algorithm>

namespace utils {

    /*
    Removes leading and trailing whitespace or any other character
    */
    std::string strip(const std::string& string, const std::string& characters) {
        size_t start = string.find_first_not_of(characters);

        if (start == std::string::npos) {
            return "";
        }

        size_t end = string.find_last_not_of(characters);

        return string.substr(start, end - start + 1);
    }

    /*
    Checks if the string is formated as a string based on if it has one of the set
    flags (eg. " or ' ) from the argument 'flags'
    */
    bool isString(const std::string& string, const std::string& flags) {
        return string.length() >= 2
            && string.front() == string.back()
            && contains(flags, string[0]);
    }

    /*
    Checks if the letter exists in the list of characters
    */
    bool contains(const std::string string, const char character) {
        for (int i = 0; string[i] != '\0'; i++) {
            if (string[i] == character)
                return true;
        }
        return false;
    }

    /*
    Returns the lower case version of an std::string
    */
    std::string lowercase(const std::string& string) {
        std::transform(string.begin(), string.end(), string.begin(),
                       [](unsigned char c) {return std::tolower(c);});
    }

    std::vector<std::string> split(const std::string& string, const char character) {
        std::vector<std::string> parts;

        if (string.empty()) {
            return parts;
        }

        size_t start = 0;
        size_t pos;

        while ((pos = string.find(character, start)) != std::string::npos) {
            parts.push_back(string.substr(start, pos - start));
            start = pos + 1;
        }

        parts.push_back(string.substr(start));

        return parts;
    }
}


