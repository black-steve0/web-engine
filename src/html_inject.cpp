#include "html_inject.hpp"

#include <fstream>
#include <sstream>

std::expected<models::Resource, err::Error>
render_html(std::filesystem::path file_path, const models::Variable& options) {
    std::ifstream in(file_path, std::ios::binary);

    if (!in) {
        return std::unexpected(err::Error(err::Code::OTHER, "Could not open file: " + file_path.string()));
    }

    std::ostringstream buffer;
    buffer << in.rdbuf();

    models::Resource resource;
    resource.route = file_path;
    resource.content = buffer.str();

    // TODO: use `options` to actually inject/template the HTML.
    // For now this just serves the file's raw contents.
    (void)options;

    return resource;
}