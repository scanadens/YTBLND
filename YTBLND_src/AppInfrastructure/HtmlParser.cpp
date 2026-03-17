/**
 * \file HtmlParser.cpp
 * \author Shamar Pennant
 * \brief HTML file parser implementation using Gumbo
 * 
 * Implements the Parser interface for HTML files using the Gumbo HTML5 parser.
 * Extracts HTML links and their text content into list<Dict<str,str>> format.
 */

#include "Parser.hpp"
#include <gumbo.h>

using namespace std;

/// HTML file parser implementation using Gumbo.
class HtmlParser : public Parser {
	public:
		HtmlParser(list<string> raw_data) {this->raw_data = raw_data;}
		HtmlParser(){}
		~HtmlParser() = default;

		list<unordered_map<string,string>> parse() {
			if (raw_data.empty()) {
				cerr << "HtmlParser: raw_data is empty" << endl;
				exit(1);
			}

			// given working with gumbo, everything is within a single item in the provided list
			string html = raw_data.front();

			//grab the output through thr gumbo parser
			GumboOutput* output = gumbo_parse(html.c_str());

			// container for returned results
			list<unordered_map<string,string>> results;

			// place extracted links from the output into results
			extract_links(output->root, results);

			// destroy ptr
			gumbo_destroy_output(&kGumboDefaultOptions, output);

			return results;
		}

		void setData(list<string>  data) override {raw_data = data;}

		int getParseId() override {return parse_id;}


	private:
		list<string> raw_data = {};
		int const parse_id = File_ID::HTML; 

		// helper funtion to extracts links and tags using gumbo
		void extract_links(GumboNode* node, list<unordered_map<string,string>>& out) {
			if (node->type != GUMBO_NODE_ELEMENT)
				return;

			GumboElement& el = node->v.element;

			if (el.tag == GUMBO_TAG_A) {
				unordered_map<string,string> row;

				// Extract href attribute
				GumboAttribute* href = gumbo_get_attribute(&el.attributes, "href");
				if (href) row["href"] = href->value;

				// Extract visible text
				row["text"] = get_text(node);

				// Tag name for debugging/consistency
				row["tag"] = "a";

				out.push_back(row);
			}

			// Recurse into children
			GumboVector* children = &el.children;
			for (unsigned i = 0; i < children->length; i++) {
				extract_links(static_cast<GumboNode*>(children->data[i]), out);
			}
    	}

    // Extract all text under a node
    string get_text(GumboNode* node) {
        if (node->type == GUMBO_NODE_TEXT)
            return node->v.text.text;

        if (node->type != GUMBO_NODE_ELEMENT)
            return "";

        string result;
        GumboVector* children = &node->v.element.children;
        for (unsigned i = 0; i < children->length; i++) {
            result += get_text(static_cast<GumboNode*>(children->data[i]));
        }
        return result;
    }
};