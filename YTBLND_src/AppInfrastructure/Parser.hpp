#pragma once

#include <list>
#include <unordered_map>
#include <string>
#include <bits/stdc++.h>
#include <vector>
#include "File_ID.hpp"

/**
 * \author Shamar Pennant
 * \brief Parses a string into a list<Dict<str,str>> object based on file format
 */
class Parser {
	public:
		virtual ~Parser() = default;

		/**
		 * Uses the values passed on intantiation to parse raw file data as a 
		 * list of unorded String to String dictionaries. Then each dictionary entry 
		 * will be of the form <"col_title" : "row_contents">. Each dictionary 
		 * entry within the list acts as a row/entry within the "table".
		 * */
		virtual std::list<std::unordered_map <std::string, std::string>> parse() = 0;

		/**Makes a copy of the parameter to this instance. Used for parse()*/
		virtual void setData(std::list<std::string> data) = 0;
		virtual int getParseId() = 0;

	protected:
		Parser() = default;
};