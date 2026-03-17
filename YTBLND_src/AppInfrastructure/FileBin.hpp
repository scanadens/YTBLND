#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include "File_ID.hpp"

/**
 * \file FileBin.hpp
 * \author Shamar Pennant
 * \brief Abstract base class for writing data to files
 * 
 * Defines the interface for persisting formatted string data to files.
 * Subclasses implement format-specific file writing operations
 * (e.g., CSV files, HTML files, etc.).
 */
/// Abstract interface for writing data to files.
class FileBin {
	public:
		/** Virtual destructor for proper cleanup in derived classes. */
		virtual ~FileBin() = default;

		/** Writes configured data to the specified output file.
		 * 
		 * Creates or overwrites the file at the configured output path and
		 * writes the configured raw data to it.
		 * 
		 * \return Status code (0 on success, non-zero on failure)
		 */
		virtual int write() = 0;

		/** Sets the output file directory path for write().
		 * \param output Directory path where file will be written
		 */
		virtual void setOutputFile(std::string output) = 0;

		/** Sets the raw data string to be written to file.
		 * \param raw Formatted data string to write
		 */
		virtual void setRawData(std::string raw) = 0;

		/** Sets the output file name (without directory path).
		 * \param fn File name for output
		 */
		virtual void setFileName(std::string fn) = 0;

	protected:
		FileBin() = default;
};