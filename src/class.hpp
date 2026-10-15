#pragma once

#include <expected>
#include <filesystem>
#include <string>
#include <vector>

namespace err {

    enum class Code {
        OK,
        UNDEFINED,
        OTHER
    };

    struct Error {
        int code = 0;
        std::string message;

        Error() = default;
        Error(int code) : code(code) {}
        Error(std::string message) : message(std::move(message)) {}

        Error(int code, std::string message)
            : code(code),
              message(std::move(message)) {}

        Error(Code code, std::string message)
            : code(static_cast<int>(code)),
              message(std::move(message)) {}
    };

    inline const Error OK{Code::OK, ""};
    inline const Error UNDEFINED{Code::UNDEFINED, "undefined"};
    inline const Error OTHER{Code::OTHER, "description unprovided"};
}

namespace models {

    struct Variable {
        std::string name;
        std::string type;
        std::string data;

        std::vector<Variable> children;

        Variable();
        Variable(std::string type, std::string data);

        void addChild(Variable child);

        std::expected<bool, err::Error> getBoolean() const;

        std::expected<std::string, err::Error> getString() const;

        std::expected<std::vector<std::string>, err::Error> getList() const;

        const Variable* getChild(const std::string& childName) const;
    };

    struct Resource {
        std::filesystem::path route;
        std::string content;
    };

    struct Response {
        unsigned int status;
        std::string mime;
        Resource resource;
    };

}