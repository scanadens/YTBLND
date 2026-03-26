#include "WatchLaterParser.hpp"

#include <memory>

WatchLaterParser::WatchLaterParser(const std::string& filePath) : filePath(filePath) {
	parser_id = File_ID::CSV;
}

std::list<Video> WatchLaterParser::parse() {
    DataExtractor extractor(
        std::make_unique<CsvSource>(filePath),
        std::make_unique<CsvParser>()
    );

    auto rows = extractor.extract();
    std::list<Video> videos;

    for (const auto& row : rows) {
        auto it = row.find("Video ID");
        if (it != row.end() && !it->second.empty()) {
            videos.emplace_back(
                it->second,
                "",
                "",
                "",
                0,
                std::list<std::string>{}
            );
        }
    }

    return videos;
}

int WatchLaterParser::getParserId() {return parser_id;}
