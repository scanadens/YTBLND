#pragma once

#include <list>
#include <unordered_map>
#include <string>
#include "File_ID.hpp"

/**
 * \file FileFormatter.hpp
 * \author Shamar Pennant
 * \brief Abstract base class for formatting structured data to file format
 * 
 * Converts list<Dict<str,str>> structured data into format-specific string
 * representation (e.g., CSV, HTML, JSON). Subclasses implement format-specific
 * formatting logic for different file types.
 *//// Abstract interface for formatting structured data to file format.class FileFormatter {
	public:
		/** Virtual destructor for proper cleanup in derived classes. */
		virtual ~FileFormatter() = default;

		/** Formats structured data into format-specific string representation.
		 * 
		 * Converts list<Dict<str,str>> configured via setData() into a properly
		 * formatted string for writing to file (e.g., CSV with commas and newlines).
		 * 
		 * \return Formatted string ready for file output
		 */
		virtual std::string format() = 0;

		/** Sets the structured data to be formatted by format().
		 * \param data List of dictionaries representing data rows
		 */
		virtual void setData(std::list<std::unordered_map<std::string, std::string>> data) = 0;

	protected:
		FileFormatter() = default;
};