/**
 * \file CsvSource.cpp
 * \brief Implementation for CsvSource.
 * \author Shamar Pennant
 */

#include "CsvSource.hpp"

using namespace std;

CsvSource::CsvSource(string src) : src(src) {
	source_id = File_ID::CSV;
}

list<string> CsvSource::read() {
	if (src.empty()) { // in the event that src is empty
		cerr << "error: src string is empty in CsvSource";
		exit(1);
	} 

	// returned string container
	list<string> ret;

	// creating (in) file stream with given file source path
	fstream read_file_in(src, ios::in);
	if (!read_file_in.is_open()) {
		cerr << "unable to open " << src << " within CsvSource" << endl;
		exit(1);
	}

	if (!read_file_in.is_open()) {
		cerr << "error: could not open file: " << src;
		exit(1);
	}

	// holds file line
	string line = "";

	// reading the opened file stream into a temporary string
	while (getline(read_file_in, line)) {ret.push_back(line);}

	return ret;
}

void CsvSource::setSource(string src) {this->src = src;}
int CsvSource::getSourceId() {return source_id;}
