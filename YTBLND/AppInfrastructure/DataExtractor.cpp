/**
 * \author Shamar Pennant
 */

#include "DataExtractor.hpp"

using namespace std;

DataExtractor::DataExtractor(unique_ptr<FileSource> src, unique_ptr<Parser> p) {
	/*
		searches both the local FileSource and Parser vectors
		looking for matching id's to match the param passed in.
		If they cant be found, or the vector is empty, just 
		push_back the new param
	*/

	if (src_formats.empty() || parsers.empty()) {
		// check if either vectors are empty
		this->src_formats.push_back(src);
		this->parsers.push_back(p);
	} else { // of neither are empty
		// check if source vector contains the given FileSource.
		// if not, then push into vector
		FileSource* temp_src = locate_src(src.get()->getSourceId());
		if (temp_src != nullptr) {
			source_id_in_use = temp_src->getSourceId();
		} else {
			src_formats.push_back(src);
			source_id_in_use = src.get()->getSourceId();
		}
		// check if parse vector contains the given Parser
		// if not then push into vector
		Parser* temp_parse = locate_parser(p.get()->getParseId());
		if (temp_parse != nullptr) {
			parse_id_in_use = temp_parse->getParseId();
		} else {
			parsers.push_back(p);
			parse_id_in_use = p.get()->getParseId();
		}
	}
}

DataExtractor::~DataExtractor() {} // TODO: finalize deconstructor

FileSource* DataExtractor::locate_src(int id) {
	for (auto& ptr : src_formats) {
		if (ptr->getSourceId() == id) {return ptr.get();}
	}

	// return nullptr if can't be found
	return nullptr;
}

Parser* DataExtractor::locate_parser(int id) {
	for (auto& ptr : parsers) {
		if (ptr->getParseId() == id) {return ptr.get();}
	}

	// if can't be found, return null ptr
	return nullptr;
}

list<unordered_map<string, string>> DataExtractor::extract() {
	// check if either id's are out of bounds of their respective vectors
	if (source_id_in_use == -1 || source_id_in_use >= src_formats.size()) {
		cerr << "One of the provided Id's is out of bounds:\nsrc_id = " << source_id_in_use << "\nparse_id = " << parse_id_in_use;
		exit(1);
	} 

	// check if id's match (how we know they match formats)
	if (source_id_in_use != parse_id_in_use) {
		cerr << "selected FileSource and Parser id's don't match: \nsrc_id = " << source_id_in_use << "\nparse_id = " << parse_id_in_use;
		exit(1);
	}

	// grab the pointers for the needed FileSource and Parser
	FileSource* tmp_src_ptr = locate_src(source_id_in_use);
	Parser* tmp_parse_ptr = locate_parser(parse_id_in_use);

	if (tmp_src_ptr == nullptr || tmp_parse_ptr == nullptr) {
		cerr << "Unable to find FileSource or Parser in extract():\nsrc_id = " << source_id_in_use << "\nparse_id = " << parse_id_in_use;
		exit(1);
	}

	// --- assuming the both source and parser are already configured ---

	// grab the raw data from the file
	list<string> raw_data = tmp_src_ptr->read();
	// format the data with the repsective parser
	list<unordered_map<string, string>> formatted_data = tmp_parse_ptr->parse();

	return formatted_data;
}

void DataExtractor::addParser(unique_ptr<Parser> parser) {
	if (locate_parser(parser.get()->getParseId()) == nullptr) {parsers.push_back(parser);}
}

void DataExtractor::addSource(unique_ptr<FileSource> src) {
	if (locate_src(src.get()->getSourceId()) == nullptr) {src_formats.push_back(src);}
}