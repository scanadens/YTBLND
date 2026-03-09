#include <iostream>
#include <fstream>
#include <string>

class FileBin {
	public:
		virtual ~FileBin() = default;

		/**Uses the pased parameters on instantiation 
		 * to create/find a file and place new data inside.
		*/
		virtual int write() = 0;

		/**Copies the param to the respective local 
		 * version for write() */
		virtual void setOutputFile(std::string output) = 0;

		/**Copies the param to the respective local 
		 * version for write() */
		virtual void setRawData(std::string raw) = 0;

		/**Copies the param to the respective local 
		 * version for write() */
		virtual void setFileName(std::string fn) = 0;

	protected:
		FileBin() = default;
};