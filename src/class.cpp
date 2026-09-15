#include <string>
#include <tuple>
#include <vector>
#include <filesystem>
#include "utils.cpp"

namespace err {

    enum class Code {
        OK,
        UNDEFINED,
        OTHER
    };

    struct Error {
        int code = 0;
        std::string message = "";

        Error() = default;
        Error(int code) : code(std::move(code)) {};
        Error(std::string message) : message(std::move(message)) {};
        
        Error(int code, std::string message) 
        :code   (std::move(code)),
         message(std::move(message)) {};

        Error(Code code, std::string message)
            : code  (static_cast<int>(code)),
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

        Variable(std::string type, std::string data)
        :type(std::move(type)),
         data(std::move(data)) 
        {};

        std::expected<bool, err::Error> getBoolean() const {
            std::string value = utils::lowercase(data);

            if (value == "true")
                return true;

            if (value == "false")
                return false;

            return std::unexpected{err::UNDEFINED};
        }

        std::expected<std::string, err::Error> getString() const {
            return data;
        }
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