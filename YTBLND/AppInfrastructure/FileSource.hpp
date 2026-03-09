#include <string>
#include <iostream>
#include <fstream>

class FileSource {
	public:
		virtual ~FileSource() = default;

		/** reads the set path to file on instantiation. 
		Returning a string of raw file data */
		virtual std::string read() = 0;

		/** Copies the given pramater to a local version 
		 * for read() to use later */
		virtual void setSource(std::string src) = 0;

	protected:
		FileSource() = default;
};