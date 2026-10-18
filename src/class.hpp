#pragma once

#include "httplib.h"
#include <expected>
#include <filesystem>
#include <string>
#include <vector>

namespace DEBUG {
    inline bool enabled = true;
}

#define DEBUG_LOG(x) \
    do { if (DEBUG::enabled) { x; } } while (0)

namespace err {

    enum class Code {
        OK = 200,
        UNDEFINED,
        OTHER,
        NONE,
        NOT_FOUND = 404,
        UNAUTHORIZED = 401,
        FORBIDDEN = 403,
        BAD_REQUEST = 400,
        INTERNAL_ERROR = 500,
    };

    struct Error {
        int code = 200;
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
    inline const Error BAD_REQUEST{Code::BAD_REQUEST, "Bad request"};
    inline const Error UNAUTHORIZED{Code::UNAUTHORIZED, "Unauthorized"};
    inline const Error FORBIDDEN{Code::FORBIDDEN, "Forbidden"};
    inline const Error NOT_FOUND{Code::NOT_FOUND, "Object was not found"};
    inline const Error INTERNAL_ERROR{Code::INTERNAL_ERROR, "Unexpected server error"};
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
        std::string mimeType;
        Resource resource;
    };

    struct URL {
        std::string fragment;
        std::string extention;
        std::string parent;
        std::string query;
        std::string name;

        void print() const;
    };

}
