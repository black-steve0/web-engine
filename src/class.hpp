#pragma once

#include "httplib.h"
#include <expected>
#include <filesystem>
#include <string>
#include <vector>

namespace err {

    enum class Code {
        OK,
        UNDEFINED,
        OTHER,
        NONE,
        NOT_FOUND,
        UNAUTHORIZED,
        FORBIDDEN,
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

    inline const Error OK{Code::OK, "ok"};
    inline const Error UNDEFINED{Code::UNDEFINED, "Undefined"};
    inline const Error OTHER{Code::OTHER, "Description unprovided"};
    inline const Error NONE{Code::NONE, "Object does not exist"};
    inline const Error NOT_FOUND{404, "Object was not found"};
    inline const Error UNAUTHORIZED{401, "Unauthorized"};
    inline const Error FORBIDDEN{403, "Forbidden"};
    inline const Error INTERNAL_ERROR{500, "Unexpected server error"};
}

namespace models {

    struct Variable {
        std::string name;
        std::string type;
        std::string data;

        std::vector<Variable> children;

        Variable();
        Variable(std::string type, std::string data);
        Variable(std::string name, std::string type, std::string data);
        Variable(std::string name, std::string type, std::string data, std::vector<Variable> children);

        void addChild(Variable child);

        std::expected<bool, err::Error> getBoolean() const;

        std::expected<std::string, err::Error> getString() const;

        std::expected<std::vector<std::string>, err::Error> getList() const;

        std::expected<Variable, err::Error> getChild(const std::string& childName) const;
        const std::vector<Variable>& getChildren() const;

        std::string dump(int indent = 0) const;
        void print() const;
        friend std::ostream& operator<<(std::ostream& os, const Variable& v);
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