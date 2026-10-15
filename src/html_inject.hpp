#pragma once

#include <expected>
#include <filesystem>
#include "class.hpp"

std::expected<models::Resource, err::Error>
render_html(std::filesystem::path file_path, const models::Variable& options);