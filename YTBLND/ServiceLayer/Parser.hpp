#include <list>
#include <unordered_map>
#include <string>

class Parser {
	public:
		virtual ~Parser() = default;

		/**Uses the values passed on intantiation to parse raw file data as a 
		list of unorded String to String dictionaries.*/
		virtual std::list<std::unordered_map <std::string, std::string>> parse() = 0;

		/**Makes a copy of the parameter to this instance. Used for parse()*/
		virtual void setData(std::string data) = 0;

	protected:
		Parser() = default;
};