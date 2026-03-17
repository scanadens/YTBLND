/**
 * \file HtmlSource.cpp
 * \author Shamar Pennant
 * \brief HTML file source/reader implementation
 * 
 * Implements the FileSource interface for reading HTML files.
 * Reads entire HTML file contents as a single string in list<string>.
 * Note: Returns list with single item due to Gumbo parser requirements.
 */

#include "FileSource.hpp"

using namespace std;

/// HTML file source/reader implementation.
class HtmlSource : public FileSource {
	public:
		HtmlSource(std::string s) {src_path = s;}
		~HtmlSource() = default;

		list<string> read() override {
			// check if src is empty
			if (src_path.empty()) {
				cerr << "html source path is empty" << endl;
				exit(1);
			}

			// open a file stream 
			ifstream read_file_in(src_path, ios::in);
			if (!read_file_in.is_open()) {
				cerr << "unable to open " << src_path << " within HtmlSource" << endl;
				exit(1);
			}

			// retrieve file contents
			stringstream buff;
			buff << read_file_in.rdbuf();

			return {buff.str()};
		}

		int getSourceId() {return source_id;}
		void setSource(string new_source) {src_path = new_source;}

	private:
		string src_path = " ";
		int const source_id = File_ID::HTML;
};