#include "FileSource.hpp"

using namespace std;

/**
 * \author Shamar Pennant
 * \brief Pulls raw file contents from a HTML file
 * 
 * Pulls raw data from an HTML file. Works slightly differently compared to CsvSource. 
 * Due to it's dependency on gumbo, read() still returns a list<string> obj, but 
 * everything is placed as a single string -- and thus, list<string> only has one item
 */
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