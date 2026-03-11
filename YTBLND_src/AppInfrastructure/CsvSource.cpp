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

			// ensure path contains no embedded null characters
			if (src.find('\0') != string::npos) {
				cerr << "error: src string contains embedded null character";
				exit(1);
			}

			// returned string
			list<string> ret;

			// creating (in) file stream with given file source path
			fstream read_file_in(src, ios::in);

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

		void setSource(string src) override {this->src = src;}
		void setSourceId(int id) override {this->source_id = id;}
		int getSourceId() override {return source_id;}

	private:
		string src = ""; // holds the file path
		int source_id = -1; // holds this objects id to match with a Parser object
};
