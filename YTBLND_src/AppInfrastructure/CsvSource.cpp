/**
 * \author Shamar Pennant
 */

#include "FileSource.hpp"

using namespace std;

class CsvSource : public FileSource {
	public:
		CsvSource(string src) {
			this-> src = src;
		}
		~CsvSource() override = default;

		list<string> read() override {
			if (src.empty()) { // in the event that src is empty
				cerr << "error: src string is empty in CsvSource";
				exit(1);
			} 

			// ensure the path is null terminated
			int num = 0;
			// sourced code for this bit here > https://www.geeksforgeeks.org/cpp/string-find-in-cpp/
			int pos = -1;
			while ((pos = src.find("\0", pos++)) != string::npos) {
				if (num >= 2) {
					cerr << "error: src string is null terminated more than once";
					exit(1);
				} else {num++;}
			}

			// if there was no null terminator found in string
			if (num == 0) {
				cerr << "error: src string is not null terminated";
				exit(1);
			}

			// returned string
			list<string> ret;

			// creating (in) file stream with given file source path 
			fstream read_file_in(src, ios::in);

			// holds file line
			string line = "";

			// reading the opened file stream into a temporary string
			while (getline(read_file_in, line)) {ret.push_back(line);}

			return ret;
		}

		void setSource(string src) override {this->src = src;}
		void setSourceId(int id) override {this->source_id = id;}
		int getSourceId() override {return source_id;}

	private:
		string src = ""; // holds the file path
		int source_id = -1; // holds this objects id to match with a Parser object
};