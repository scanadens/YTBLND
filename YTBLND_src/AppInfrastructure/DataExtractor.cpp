/**
 * \file DataExtractor.cpp
 * \author Shamar Pennant
 * \brief DataExtractor class implementation
 * 
 * Implements the DataExtractor class which coordinates FileSource and Parser
 * objects to read and parse file data into structured format.
 */

#include "DataExtractor.hpp"

using namespace std;

DataExtractor::DataExtractor(unique_ptr<FileSource> src, unique_ptr<Parser> p) {
	// change ownership of the param to this objcts instance
	src_format = move(src);
	par_format = move(p);
}

DataExtractor::~DataExtractor() {} 

list<unordered_map<string, string>> DataExtractor::extract() {
	// grab the respective file format ids 
	int source_id_in_use = src_format->getSourceId();
	int parse_id_in_use = par_format->getParseId();

	// check if either id's are out of bounds (they shouldn't be)
	if (source_id_in_use <= -1 || parse_id_in_use <= -1) {
		cerr << "One of the provided ID's is out of bounds:\nsrc_id = " << source_id_in_use << "\nparse_id = " << parse_id_in_use;
		exit(1);
	}

	// check if id's match (how we know they match formats)
	if (source_id_in_use != parse_id_in_use) {
		cerr << "selected FileSource and Parser id's don't match: \nsrc_id = " << source_id_in_use << "\nparse_id = " << parse_id_in_use;
		exit(1);
	}

	// ensure neither pointers are null
	if (src_format == nullptr || par_format == nullptr) {
		cerr << "Unable to find FileSource or Parser in extract():\nsrc_id = " 
			 << source_id_in_use << ", " << (src_format == nullptr ? "contains nothing (nullptr)" : "has obj")
			 << "\nparse_id = " 
			 << parse_id_in_use << ", " << (par_format == nullptr ? "contains nothing (nullptr)" : "has obj");
		exit(1);
	}

	// --- assuming the both source and parser are already configured ---

	// grab the raw data from the file
	auto raw_data = src_format->read();
	// feed the raw data into the given parser
	par_format->setData(raw_data);
	// format the data with the repsective parser
	list<unordered_map<string, string>> formatted_data = par_format->parse();

	return formatted_data;
}

void DataExtractor::setParser(unique_ptr<Parser> parser) {
	par_format = move(parser);
}

void DataExtractor::setSource(unique_ptr<FileSource> src) {
	src_format = move(src);
}
