#pragma once
#include <string>
#include <iostream>
#include <fstream>
#include <list>
#include "File_ID.hpp"
#include <sstream>

/**
 * \file FileSource.hpp
 * \author Shamar Pennant
 * \brief Abstract base class for reading file source data
 * 
 * Defines the interface for file reading operations across different file formats.
 * Subclasses implement format-specific source reading (CSV, HTML, etc.).
 */
/** Abstract interface for reading file source data. */
/**
 * \class FileSource
 * \brief FileSource class declaration.
 */
class FileSource {
	public:
		virtual ~FileSource() = default;

		/** Reads the configured file source path and returns raw file data.
		 * \return List of strings containing raw file data (line-by-line or complete content)
		 */
		virtual std::list<std::string> read() = 0;

		/** Sets the file source path for read() to use.
		 * \param src File path to configure as the source for reading
		 */
		virtual void setSource(std::string src) = 0;
		
		/** Gets the file format ID associated with this source.
		 * \return Integer ID representing the file format (e.g., File_ID::CSV)
		 */
		virtual int getSourceId() = 0;

	protected:
		FileSource() = default;
};
