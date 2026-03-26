/**
 * \file CsvParser.cpp
 * \author Shamar Pennant
 * \brief CSV file parser implementation
 * 
 * Implements the Parser interface for CSV files. Parses comma-separated values
 * into list<Dict<str,str>> format where the first row is treated as column headers.
 */

# pragma once

#include "Parser.hpp"

class CsvParser : public Parser {
	public:
		CsvParser();
		CsvParser(std::list<std::string> raw_data);
		~CsvParser();

		std::list<std::unordered_map<std::string, std::string>> parse() override;
		void setData(std::list<std::string> data) override;
		int getParseId() override;
	
	private:
		std::list<std::string> raw_data;
		int parse_id;
};