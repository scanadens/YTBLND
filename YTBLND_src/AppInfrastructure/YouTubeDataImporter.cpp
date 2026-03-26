#include "YouTubeDataImporter.hpp"

#include "WatchHistoryParser.hpp"
#include "WatchLaterParser.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>

std::string YouTubeDataImporter::lowerExtension(const std::string& filePath) {
    std::size_t dotPos = filePath.find_last_of('.');
    if (dotPos == std::string::npos) {
        return "";
    }

    std::string ext = filePath.substr(dotPos);
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext;
}

std::list<Video> YouTubeDataImporter::import(const std::string& filePath) const {
    const std::string ext = lowerExtension(filePath);

    if (ext == ".csv") {
        return WatchLaterParser(filePath).parse();
    }

    if (ext == ".html" || ext == ".htm") {
        return WatchHistoryParser(filePath).parse();
    }

    throw std::invalid_argument(
        "YouTubeDataImporter: unsupported file extension for path: " + filePath
    );
}