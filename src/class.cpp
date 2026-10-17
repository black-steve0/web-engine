#include "class.hpp"
#include "utils.hpp"
#include "sstream"
#include "iostream"
#include <expected>

namespace models {

    namespace {
        // Empty strings print as null, everything else as an escaped "string"
        std::string quoteOrNull(const std::string& s) {
            if (s.empty()) return "null";
    
            std::string out = "\"";
            for (char c : s) {
                switch (c) {
                    case '"':  out += "\\\""; break;
                    case '\\': out += "\\\\"; break;
                    case '\n': out += "\\n";  break;
                    case '\t': out += "\\t";  break;
                    default:   out += c;
                }
            }
            return out + "\"";
        }
    }
    
    std::string Variable::dump(int indent) const {
        std::string pad(indent * 2, ' ');
        std::string in((indent + 1) * 2, ' ');
    
        std::ostringstream o;
        o << "Variable {\n";
        o << in << "name: "        << quoteOrNull(name) << ",\n";
        o << in << "type: "        << quoteOrNull(type) << ",\n";
        o << in << "data: "        << quoteOrNull(data) << ",\n";
        o << in << "hasChildren: " << (children.empty() ? "false" : "true") << ",\n";
        o << in << "children: ";
    
        if (children.empty()) {
            o << "null";
        } else {
            o << "[\n";
            for (size_t i = 0; i < children.size(); i++) {
                o << in << "  " << children[i].dump(indent + 2);
                if (i + 1 < children.size()) o << ",";
                o << "\n";
            }
            o << in << "]";
        }
    
        o << "\n" << pad << "}";
        return o.str();
    }
    
    void Variable::print() const {
        std::cout << dump() << std::endl;   // endl flushes, so it survives a crash
    }
    
    std::ostream& operator<<(std::ostream& os, const Variable& v) {
        return os << v.dump();
    }

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
        if (children.empty()) {
            return std::unexpected{err::NONE};
        }
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
