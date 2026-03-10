#pragma once

#include "FileSource.hpp"
#include "Parser.hpp"
#include <vector>
#include <memory>

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
class DataExtractor {
	public:
		DataExtractor(std::unique_ptr<FileSource> src, std::unique_ptr<Parser> p);
		~DataExtractor();

		/* given the param passed in the constructor, it extracts raw file data and parses it
		into list<Dict<str,str>>. Using each the id's of each object to match the respective
		FileSource to Parser. 
		Upon creating either a FileSource or Parser object to pass into 
		DataExtractor, their ID's must match one another for this function to identify whether 
		their formats match as well. They can be any interger, so long as they both match one 
		another.
		Note, this function assumes both objects are already configured as needed
		before passing to the DataExtractor or using setters*/
		std::list<std::unordered_map<std::string, std::string>> extract();
		
		// adds FileSource to respective vector if it's not there already
		void addSource(std::unique_ptr<FileSource> src);

		// adds Parser to the respective vector if it's not there already
		void addParser(std::unique_ptr<Parser> parser);

	private:
		std::vector<std::unique_ptr<FileSource>> src_formats;
		std::vector<std::unique_ptr<Parser>> parsers;
		int source_id_in_use = -1;
		int parse_id_in_use = -1;

		// locates the given id in src_formats
		FileSource* locate_src(int id);
		// locates the given id in parsers
		Parser* locate_parser(int id);
};