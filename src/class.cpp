#include "class.hpp"
#include "utils.hpp"
#include <expected>

namespace models {

    Variable::Variable() : type("block") {}

    Variable::Variable(std::string type, std::string data)
        : type(std::move(type)), data(std::move(data)) {}

    Variable::Variable(std::string name, std::string type, std::string data)
        : name(std::move(name)),
        type(std::move(type)),
        data(std::move(data)) {}

    Variable::Variable(std::string name, std::string type, std::string data, std::vector<Variable> children)
        : name(std::move(name)),
        type(std::move(type)),
        data(std::move(data)),
        children(std::move(children)) {}

    void Variable::addChild(Variable child) {
        children.push_back(std::move(child));
    }

    std::expected<bool, err::Error> Variable::getBoolean() const {
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

    std::expected<std::string, err::Error> Variable::getString() const {
        if (type != "string") {
            return std::unexpected{err::UNDEFINED};
        }

        return data;
    }

    std::expected<std::vector<std::string>, err::Error> Variable::getList() const {
        if (type != "list") {
            return std::unexpected{err::UNDEFINED};
        }

        std::vector<std::string> result;

        for (const auto& child : children) {
            result.push_back(child.data);
        }

        return result;
    }

    std::expected<Variable, err::Error> Variable::getChild(const std::string& childName) const {
        for (const auto& child : children) {
            if (child.name == childName) {
                return child;
            }
        }

        return std::unexpected{err::NONE};
    }

    const std::vector<Variable>& Variable::getChildren() const {
        return children;
    }

    

}