/**
 * @author Shamar Pennant
 * \brief Parses csv file contents into a single string
 */

using namespace std;

#include "Parser.hpp"
#include "FileSource.hpp"

class CsvParser : public Parser {
	public:
		CsvParser(list<string> raw_data) {
			this->raw_data = raw_data;
		} 
		~CsvParser();

		list<unordered_map<string, string>> parse() {
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
				// grab the next line
				string line = raw_data.front();

				if (line_num == 0) { 
					/*
					When on the first line (the col titles)
					save their names within col_titles to be 
					used later. delimiting the row to get each 
					column name
					*/
					stringstream temp_ss(line);
					string t = "";
					while (getline(temp_ss, t, ',')) {
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
						row_contents[col_titles.at(num_col++)] = t;
					}

					// add the dictionary as a line to ret
					ret.push_back(row_contents);
				}
			}
		}

		void setData(list<string> data) {
			this->raw_data = data;
		}

	private:
		list<string> raw_data;
};