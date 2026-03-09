#include <list>
#include <unordered_map>
#include <string>

class FileFormatter {
	public:
		virtual ~FileFormatter() = default;

		/**Uses param's from intantiation to format raw file
		 * data into an accesible ADT
		 */
		virtual std::string format() = 0;

		/**Makes a copy of the given param to this object */
		virtual void setData(std::list<std::unordered_map<std::string, std::string>> data) = 0;

	protected:
		FileFormatter() = default;
};