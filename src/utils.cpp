#include <string>
#include <cctype>
#include <algorithm>

namespace utils {

    /*
    Removes leading and trailing whitespace or any other character
    */
    std::string strip(std::string string, char character) {
        int start;
        int end;
        for (int i = 0; i < string.length(); i++) {
            if (string[i] != character) {
                start = i;
                break;
            }
        }

        for (int i = string.length() -1; i >= 0; i--) {
            if (string[i] != character) {
                end = i;
                break;
            }
        }

        return string.substr(start, end - start + 1);
    }

    /*
    Checks if the string is formated as a string based on if it has one of the set
    flags (eg. " or ' ) from the argument 'flags'
    */
    bool isString(const std::string& string, const char* flags) {
        return string.length() > 2
            && string.front() == string.back()
            && contains(flags, string[0]);
    }

    /*
    Checks if the letter exists in the list of characters
    */
    bool contains(const char* str, char character) {
        for (int i = 0; str[i] != '\0'; i++) {
            if (str[i] == character)
                return true;
        }
        return false;
    }

    /*
    Returns the lower case version of an std::string
    */
    std::string lowercase(std::string string) {
        std::transform(string.begin(), string.end(), string.begin(),
                       [](unsigned char c) {return std::tolower(c);});
    }
}


