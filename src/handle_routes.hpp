#pragma once

#include <expected>
#include <filesystem>
#include <string>

#include "class.hpp"

std::expected<std::filesystem::path, err::Error> not_found();

std::expected<std::filesystem::path, err::Error>
getFilePath(std::filesystem::path route, std::string type);

// std::expected<models::Resource, err::Error> getFile(std::filesystem::path route);
std::expected<models::Resource, err::Error> getResource(std::filesystem::path path);
std::expected<std::filesystem::path, err::Error> getPath(models::URL url);
std::expected<models::Response, err::Error> get(httplib::Request request);

std::expected<void, err::Error> updater();

std::expected<void, err::Error> init();
