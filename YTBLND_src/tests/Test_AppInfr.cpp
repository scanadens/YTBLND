/**
 * \file Test_AppInfr.cpp
 * \brief Unit tests for AppInfrastructure parsers, sources, and import pipeline.
 * \author Shamar Pennant
 *
 * \details
 * Validates AppInfrastructure behavior for:
 * - File_ID constant stability and expected values.
 * - CsvParser parsing rules, including headers and row mapping.
 * - CsvSource file loading behavior and error handling.
 * - DataExtractor source-parser wiring and compatibility checks.
 * - WatchLaterParser and WatchHistoryParser extraction correctness.
 * - YouTubeDataImporter routing by file extension and unsupported-type errors.
 */

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <list>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "../AppInfrastructure/CsvParser.hpp"
#include "../AppInfrastructure/CsvSource.hpp"
#include "../AppInfrastructure/DataExtractor.hpp"
#include "../AppInfrastructure/File_ID.hpp"
#include "../AppInfrastructure/WatchHistoryParser.hpp"
#include "../AppInfrastructure/WatchLaterParser.hpp"
#include "../AppInfrastructure/YouTubeDataImporter.hpp"

namespace {

    const std::string kCsvHeader = "name,email";
    const std::string kCsvRow1 = "Dijon,djmustard@spotty.com";
    const std::string kCsvRow2 = "Maya,maya@example.com";

    std::filesystem::path writeFile(const std::filesystem::path& path, const std::string& content) {
        std::filesystem::create_directories(path.parent_path());
        std::ofstream out(path);
        EXPECT_TRUE(out.is_open());
        out << content;
        out.close();
        return path;
    }

    std::filesystem::path testDataDir() {
        return std::filesystem::temp_directory_path() / "ytblnd_appinfr_tests_tmp";
    }

    std::list<std::string> sampleCsvLines() {
        return {kCsvHeader, kCsvRow1};
    }

    std::vector<std::string> collectVideoIds(const std::list<Video>& videos) {
        std::vector<std::string> ids;
        ids.reserve(videos.size());
        for (const auto& v : videos) {
            ids.push_back(v.getVideoID());
        }
        return ids;
    }

} // namespace

class AppInfrastructureTest : public ::testing::Test {
    protected:
        std::filesystem::path tmpDir;

        void SetUp() override {
            tmpDir = testDataDir();
            std::filesystem::create_directories(tmpDir);
        }

        void TearDown() override {
            std::error_code ec;
            std::filesystem::remove_all(tmpDir, ec);
        }
};

TEST_F(AppInfrastructureTest, FileIdConstantsAreStable) {
    EXPECT_EQ(File_ID::CSV, 1);
    EXPECT_EQ(File_ID::HTML, 2);
    EXPECT_EQ(File_ID::JSON, 3);
}

TEST_F(AppInfrastructureTest, CsvSourceReadsAllLines) {
    const auto filePath = writeFile(tmpDir / "source_read.csv", kCsvHeader + "\n" + kCsvRow1 + "\n");

    CsvSource source(filePath.string());
    const auto lines = source.read();

    ASSERT_EQ(lines.size(), 2);
    auto it = lines.begin();
    EXPECT_EQ(*it, kCsvHeader);
    ++it;
    EXPECT_EQ(*it, kCsvRow1);
    EXPECT_EQ(source.getSourceId(), File_ID::CSV);
}

TEST_F(AppInfrastructureTest, CsvSourceSetSourceUpdatesReadTarget) {
    const auto fileA = writeFile(tmpDir / "a.csv", "id,name\n1,Alice\n");
    const auto fileB = writeFile(tmpDir / "b.csv", "id,name\n2,Bob\n");

    CsvSource source(fileA.string());
    source.setSource(fileB.string());
    const auto lines = source.read();

    ASSERT_EQ(lines.size(), 2);
    EXPECT_EQ(lines.back(), "2,Bob");
}

TEST_F(AppInfrastructureTest, CsvParserParsesRowsUsingHeaderKeys) {
    CsvParser parser(sampleCsvLines());
    const auto rows = parser.parse();

    ASSERT_EQ(rows.size(), 1);
    const auto& row = rows.front();
    EXPECT_EQ(row.at("name"), "Dijon");
    EXPECT_EQ(row.at("email"), "djmustard@spotty.com");
    EXPECT_EQ(parser.getParseId(), File_ID::CSV);
}

TEST_F(AppInfrastructureTest, CsvParserSetDataReplacesInput) {
    CsvParser parser;
    parser.setData({"name,email", "Maya,maya@example.com"});

    const auto rows = parser.parse();
    ASSERT_EQ(rows.size(), 1);
    EXPECT_EQ(rows.front().at("name"), "Maya");
}

TEST_F(AppInfrastructureTest, DataExtractorExtractsCsvThroughSourceAndParser) {
    const auto filePath = writeFile(tmpDir / "extract.csv", kCsvHeader + "\n" + kCsvRow1 + "\n");

    DataExtractor extractor(
        std::make_unique<CsvSource>(filePath.string()),
        std::make_unique<CsvParser>()
    );

    const auto rows = extractor.extract();
    ASSERT_EQ(rows.size(), 1);
    const auto& row = rows.front();
    EXPECT_EQ(row.at("name"), "Dijon");
    EXPECT_EQ(row.at("email"), "djmustard@spotty.com");
}

TEST_F(AppInfrastructureTest, DataExtractorCanSwapSourceAndParser) {
    const auto fileA = writeFile(tmpDir / "swap_a.csv", "name,email\nA,a@example.com\n");
    const auto fileB = writeFile(tmpDir / "swap_b.csv", "name,email\nB,b@example.com\n");

    DataExtractor extractor(
        std::make_unique<CsvSource>(fileA.string()),
        std::make_unique<CsvParser>()
    );

    auto rows = extractor.extract();
    ASSERT_EQ(rows.front().at("name"), "A");

    extractor.setSource(std::make_unique<CsvSource>(fileB.string()));
    extractor.setParser(std::make_unique<CsvParser>());
    rows = extractor.extract();
    ASSERT_EQ(rows.front().at("name"), "B");
}

TEST_F(AppInfrastructureTest, WatchHistoryParserExtractsAndDeduplicatesVideoIds) {
    const std::string html =
        "<html><body>"
        "<a href=\"https://www.youtube.com/watch?v=abc123&list=PL1\">one</a>"
        "<a href=\"https://youtu.be/xyz789?t=30\">two</a>"
        "<a href=\"https://www.youtube.com/watch?v=abc123\">dup</a>"
        "<a href=\"https://example.com/not-video\">skip</a>"
        "</body></html>";

    const auto filePath = writeFile(tmpDir / "watch_history.html", html);

    WatchHistoryParser parser(filePath.string());
    EXPECT_EQ(parser.getParserId(), File_ID::HTML);

    const auto videos = parser.parse();
    const auto ids = collectVideoIds(videos);

    ASSERT_EQ(ids.size(), 2);
    EXPECT_EQ(ids[0], "abc123");
    EXPECT_EQ(ids[1], "xyz789");
}

TEST_F(AppInfrastructureTest, WatchLaterParserParsesVideoIdColumn) {
    const std::string csv =
        "Video ID,Playlist Video Creation Timestamp\n"
        "s1FkO7Tr70A,2023-12-19T00:03:12+00:00\n"
        "XuTQbOo3Y30,2024-02-05T00:12:57+00:00\n";

    const auto filePath = writeFile(tmpDir / "watch_later.csv", csv);

    WatchLaterParser parser(filePath.string());
    EXPECT_EQ(parser.getParserId(), File_ID::CSV);

    const auto videos = parser.parse();
    const auto ids = collectVideoIds(videos);
    ASSERT_EQ(ids.size(), 2);
    EXPECT_EQ(ids[0], "s1FkO7Tr70A");
    EXPECT_EQ(ids[1], "XuTQbOo3Y30");
}

TEST_F(AppInfrastructureTest, YouTubeDataImporterRoutesCsvToWatchLaterParser) {
    const std::string csv =
        "Video ID,Playlist Video Creation Timestamp\n"
        "dQw4w9WgXcQ,2024-03-01T12:00:00+00:00\n";
    const auto filePath = writeFile(tmpDir / "import.csv", csv);

    YouTubeDataImporter importer;
    const auto videos = importer.import(filePath.string());

    ASSERT_EQ(videos.size(), 1);
    EXPECT_EQ(videos.front().getVideoID(), "dQw4w9WgXcQ");
}

TEST_F(AppInfrastructureTest, YouTubeDataImporterRoutesHtmlToWatchHistoryParser) {
    const std::string html =
        "<html><body>"
        "<a href=\"https://www.youtube.com/watch?v=vid001\">v1</a>"
        "<a href=\"https://www.youtube.com/watch?v=vid002\">v2</a>"
        "</body></html>";
    const auto filePath = writeFile(tmpDir / "import_history.html", html);

    YouTubeDataImporter importer;
    const auto videos = importer.import(filePath.string());

    const auto ids = collectVideoIds(videos);
    ASSERT_EQ(ids.size(), 2);
    EXPECT_EQ(ids[0], "vid001");
    EXPECT_EQ(ids[1], "vid002");
}

TEST_F(AppInfrastructureTest, YouTubeDataImporterRejectsUnsupportedExtension) {
    const auto filePath = writeFile(tmpDir / "unsupported.txt", "plain text");

    YouTubeDataImporter importer;
    EXPECT_THROW(importer.import(filePath.string()), std::invalid_argument);
}
