#pragma once

#include <expected>
#include <filesystem>
#include <string>

#include "class.hpp"

std::expected<std::filesystem::path, err::Error> not_found();

std::expected<std::filesystem::path, err::Error>
getFilePath(std::filesystem::path route, std::string type);

std::expected<models::Resource, err::Error> getFile(std::filesystem::path route);

std::expected<void, err::Error> updater();

std::expected<void, err::Error> init();