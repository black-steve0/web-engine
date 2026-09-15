#include <string>
#include <tuple>
#include <vector>
#include <filesystem>
#include <expected>
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
        Error(int code) : code(code) {};
        Error(std::string message) : message(std::move(message)) {};

        Error(int code, std::string message)
        :code   (code),
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

    // A Variable is either a leaf (string/boolean), a list (children hold
    // unnamed string values), a route (children hold a "file" and an
    // "options" entry), or a block (children hold named entries - this is
    // how nested "{ ... }" blocks / the whole config tree are represented,
    // the equivalent of a nested dict in the Python version).
    struct Variable {
        std::string name;
        std::string type;
        std::string data;

        std::vector<Variable> children;

        // Default: an unnamed block/container (e.g. the config root).
        Variable() : type("block") {};

        // Leaf value: string or boolean.
        Variable(std::string type, std::string data)
        :type(std::move(type)),
         data(std::move(data))
        {};

        void addChild(Variable child) {
            children.push_back(std::move(child));
        }

        std::expected<bool, err::Error> getBoolean() const {
            if (type != "boolean") {
                return std::unexpected{err::UNDEFINED};
            }

            std::string value = utils::lowercase(data);

            if (value == "true")
                return true;

            if (value == "false")
                return false;

            return std::unexpected{err::UNDEFINED};
        }

        std::expected<std::string, err::Error> getString() const {
            if (type != "string") {
                return std::unexpected{err::UNDEFINED};
            }

            return data;
        }

        std::expected<std::vector<std::string>, err::Error> getList() const {
            if (type != "list") {
                return std::unexpected{err::UNDEFINED};
            }

            std::vector<std::string> result;

            for (const auto& child : children) {
                result.push_back(child.data);
            }

            return result;
        }

        // Looks up a named child (block/route member). Returns nullptr if
        // this isn't a container or the child doesn't exist.
        const Variable* getChild(const std::string& childName) const {
            for (const auto& child : children) {
                if (child.name == childName) {
                    return &child;
                }
            }

            return nullptr;
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