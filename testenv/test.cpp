#include <string>

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

#include <iostream>
int main() {
    std::string str = "    hello world     !      ";
    str = strip(str, ' ');

    std::printf("%s", str.c_str());
}