

#include "CsvParser.hpp"

using namespace std;

CsvParser::CsvParser() {parse_id = File_ID::CSV;}
CsvParser::CsvParser(list<string> raw_data) : raw_data(raw_data) {
	parse_id = File_ID::CSV;
}
CsvParser::~CsvParser() {}

list<unordered_map<string, string>> CsvParser::parse() {
	// check if raw_data is empty
	if (raw_data.empty()) {
		cerr << "error: raw data string is empty in CsvParser";
		exit(1);
	}

	// returned value at end of function
	list<unordered_map<string, string>> ret;

	// header contents
	vector<string> col_titles;

	int line_num = 0;
	// loop through the raw_data list until it's empty
	while (!raw_data.empty()) {
		// grab the next line and advance
		string line = raw_data.front();
		raw_data.pop_front();

		if (line_num == 0) { 
			/*
			When on the first line (the col titles)
			save their names within col_titles to be 
			used later. formatting each dictionary 
			contents as key = col_title and value =
			row_entry 
			*/
			stringstream temp_ss(line);
			string t = "";
			while (getline(temp_ss, t, ',')) {
				// trim any white space/newline char
				if (!t.empty() && t.back() == '\n') {t.pop_back();}
				// save col
				col_titles.push_back(t);
			}
		} else { // for any other line, they're normal rows
			int num_col = 0;
			unordered_map<string, string> row_contents;

			/* delimit the line/row and place key value pay 
			<"col_name", "row_value"> inside the dictionary*/
			stringstream temp_ss(line);
			string t = "";
			while (getline(temp_ss, t, ',')) {
				if (!t.empty() && t.back() == '\n') {t.pop_back();}
				row_contents[col_titles.at(num_col++)] = t;
			}

			// add the dictionary as a line to ret
			ret.push_back(row_contents);
		}

		line_num++;
	}
	
	return ret;
}

void CsvParser::setData(list<string> data) {this->raw_data = data;}
int CsvParser::getParseId() {return parse_id;}
