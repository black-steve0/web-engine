#include <expected>
#include <filesystem>
#include "class.cpp"

std::expected<std::filesystem::path, err::Error> not_found() {
    std::filesystem::path path = "";
    return path;
}

std::expected<std::filesystem::path, err::Error> getFilePath(std::filesystem::path route, std::string type) {

}

std::expected<models::Resource, err::Error> getFile() {

}

std::expected<void, err::Error> updater() {

}

std::expected<void, err::Error> init() {

}