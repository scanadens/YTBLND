/**
 * \author Shamar Pennant
 * \brief Parses csv file contents into a single string
 */

#include "Parser.hpp"

using namespace std;

class CsvParser : public Parser {
	public:
		CsvParser() {}
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
					vector<string> fields = parseCsvLine(line);
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

		void setData(list<string> data) override {
			this->raw_data = data;
		}
		int getParseId() override {return parse_id;}

	private:
		list<string> raw_data;
		int parse_id = File_ID::CSV;
};
