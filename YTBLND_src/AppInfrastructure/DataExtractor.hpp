#pragma once

#include "FileSource.hpp"
#include "Parser.hpp"
#include "File_ID.hpp"

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
		/* Copies FileSource and Parser obj to class member variables. Ensure that both are for 
		the same target file format*/
		DataExtractor(std::unique_ptr<FileSource> src, std::unique_ptr<Parser> p);
		~DataExtractor();

		/* given the param passed in the constructor, it extracts raw file data and parses it
		into list<Dict<str,str>>. Using each the id's of each object to match the respective
		FileSource to Parser.
		Note, this function assumes both objects are already configured as needed
		before passing to the DataExtractor or using setters*/
		std::list<std::unordered_map<std::string, std::string>> extract();
		
		void setSource(std::unique_ptr<FileSource> src);
		void setParser(std::unique_ptr<Parser> p);

	private:
		std::unique_ptr<FileSource> src_format;
		std::unique_ptr<Parser> par_format;
};