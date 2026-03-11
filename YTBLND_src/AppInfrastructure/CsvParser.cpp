/**
 * \author Shamar Pennant
 * \brief Parses csv file contents into a single string
 */

#include "Parser.hpp"

using namespace std;

class CsvParser : public Parser {
	public:
		CsvParser(list<string> raw_data) {
			this->raw_data = raw_data;
		}
		~CsvParser() override = default;

		list<unordered_map<string, string>> parse() override {
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
					used later. delimiting the row to get each
					column name
					*/
					col_titles = parseCsvLine(line);
				} else { // for any other line, they're normal rows
					vector<string> fields = parseCsvLine(line);
					unordered_map<string, string> row_contents;

					/* map each field to its column title */
					for (size_t i = 0; i < fields.size() && i < col_titles.size(); i++) {
						row_contents[col_titles[i]] = fields[i];
					}

					// add the dictionary as a line to ret
					ret.push_back(row_contents);
				}
				line_num++;
			}

			return ret;
		}

		void setData(list<string> data) override {
			this->raw_data = data;
		}

		void setParseId(int id) override {this->parse_id = id;}
		int getParseId() override {return parse_id;}

	private:
		list<string> raw_data;
		int parse_id = -1;

		// Parses a single CSV line, correctly handling quoted fields
		// e.g. `"value, with comma"` is treated as one field
		vector<string> parseCsvLine(const string& line) {
			vector<string> fields;
			string field;
			bool in_quotes = false;

			for (size_t i = 0; i < line.size(); i++) {
				char c = line[i];
				if (in_quotes) {
					if (c == '"' && i + 1 < line.size() && line[i + 1] == '"') {
						field += '"'; // escaped quote: "" -> "
						i++;
					} else if (c == '"') {
						in_quotes = false;
					} else {
						field += c;
					}
				} else {
					if (c == '"')      { in_quotes = true; }
					else if (c == ',') { fields.push_back(field); field.clear(); }
					else               { field += c; }
				}
			}
			fields.push_back(field); // push last field
			return fields;
		}
};
