/**
 * \author Shamar Pennant
 * \brief Extracts file data and parses to be a List<Dictionary<Str,Str>> format
 * 
 * Purpose of the class is to extract raw data from a file using \class FileSource 
 * sub-classes and interpret the raw data using subclasses of \class Parser.
 * Ultimately processing the data to be an accessible format within the program 
 * (List<Dictionary<Str,Str>>). Helping to ensure consistency in data retrieval 
 * from external files
 */

#pragma once

#include "FileSource.hpp"
#include "Parser.hpp"
#include <vector>
#include <memory>

class DataExtractor {
	public:
		DataExtractor(FileSource src, Parser p);
		~DataExtractor();

		/* given the param passed in the constructor, it extracts raw file data and parses it
		into list<Dict<str,str>> */
		std::list<std::unordered_map<std::string, std::string>> extract();
		void setSource(FileSource src);
		void setParser(Parser parser);

	private:
		std::vector<std::unique_ptr<FileSource>> src_formats;
		std::vector<std::unique_ptr<Parser>> p;
};