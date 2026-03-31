#pragma once

#include <list>
#include <unordered_map>
#include <string>

#include <iostream>
#include <algorithm>
#include <sstream>

#include <vector>
#include "File_ID.hpp"

/**
 * \file Parser.hpp
 * \author Shamar Pennant
 * \brief Abstract base class for parsing file data into structured format
 * 
 * Defines the interface for converting raw file data into a consistent
 * list of dictionaries format (list<Dict<string,string>>). Each dictionary
 * represents a row/entry with column headers as keys.
 */
/// Abstract interface for parsing file data into structured format.
class Parser {
	public:
		virtual ~Parser() = default;

		/**
		 * Parses raw file data into a structured list of dictionaries.
		 * 
		 * Converts raw file data configured via setData() into a list of unordered
		 * string-to-string dictionaries. Each dictionary represents a row with
		 * column titles as keys and row contents as values.
		 * 
		 * \return List of unordered maps representing parsed file rows
		 */
		virtual std::list<std::unordered_map <std::string, std::string>> parse() = 0;

		/** Sets the raw file data to be parsed by parse().
		 * \param data List of strings containing raw file data
		 */
		virtual void setData(std::list<std::string> data) = 0;
		
		/** Gets the file format ID associated with this parser.
		 * \return Integer ID representing the file format (e.g., File_ID::CSV)
		 */
		virtual int getParseId() = 0;

	protected:
		Parser() = default;
};