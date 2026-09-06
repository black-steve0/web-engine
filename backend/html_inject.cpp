#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;

extern std::string base;
extern bool force_root;

extern std::map<std::string, std::string> dirs;
extern std::map<std::string, std::string> defaults;

static std::string read_text(const fs::path& path) {
    std::ifstream file(path);

    if (!file)
        throw std::runtime_error(
            "Could not open file: " + path.string()
        );

    std::stringstream buffer;
    buffer << file.rdbuf();

    return buffer.str();
}

std::string render_html(
    const fs::path& file_path,
    std::vector<std::string> options
) {
    std::string content = read_text(file_path);

    if (force_root)
        options.insert(options.begin(), "root");

    auto html_dir = dirs.find("html");

    if (html_dir == dirs.end())
        return content;

    for (const std::string& option : options) {
        const bool prepend =
            option.rfind("--", 0) == 0;

        std::string name = option;

        while (!name.empty() && name.front() == '-')
            name.erase(name.begin());

        auto default_it = defaults.find(name);

        if (default_it == defaults.end())
            continue;

        fs::path template_path =
            fs::path(base) /
            html_dir->second /
            default_it->second;

        std::string template_content =
            read_text(template_path);

        if (prepend)
            content = template_content + content;
        else
            content += template_content;
    }

    return content;
}