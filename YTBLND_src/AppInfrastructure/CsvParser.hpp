/**
 * \file CsvParser.hpp
 * \author Shamar Pennant
 * \brief CSV file parser implementing the Parser interface.
 *
 * Parses comma-separated values into a list<Dict<string,string>> format
 * where the first row is treated as column headers.
 */

#pragma once

#include "Parser.hpp"

/** CSV file parser implementing the Parser interface. */
/**
 * \class CsvParser
 * \brief CsvParser class declaration.
 */
class CsvParser : public Parser {
    public:
        CsvParser();
        /**
         * Constructs a CsvParser pre-loaded with raw data.
         * \param raw_data Lines of raw CSV text to be parsed.
         */
        CsvParser(std::list<std::string> raw_data);
        ~CsvParser();

        /**
         * Parses the loaded raw CSV data into a list of row dictionaries.
         * The first row is used as column headers (keys).
         * \return List of maps from column name to cell value.
         */
        std::list<std::unordered_map<std::string, std::string>> parse() override;

        /**
         * Replaces the raw data to be parsed on the next parse() call.
         * \param data Lines of raw CSV text.
         */
        void setData(std::list<std::string> data) override;

        /** \return File_ID::CSV */
        int getParseId() override;

    private:
        std::list<std::string> raw_data;
        int parse_id;
};
