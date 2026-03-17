#pragma once

#include "FileBin.hpp"
#include "FileFormatter.hpp"
#include <vector>
#include <memory>
#include "File_ID.hpp"

/**
 * \file DataSaver.hpp
 * \author Shamar Pennant
 * \brief Saves structured data to files in desired format
 * 
 * Coordinates data formatting (via FileFormatter) and file output (via FileBin)
 * to persist list<Dict<str,str>> data to files. Maintains collections of
 * FileBin and FileFormatter objects to support multiple file formats.
 */
/// Coordinates FileFormatter and FileBin to save structured data to files.
class DataSaver {
	public:
		/** Constructs a DataSaver with initial FileBin and FileFormatter.
		 * \param b The FileBin implementation for file output
		 * \param ff The FileFormatter implementation for data formatting
		 */
		DataSaver(FileBin b, FileFormatter ff);
		
		/** Destructor. Cleans up owned FileBin and FileFormatter objects. */
		~DataSaver();

		/** Formats and writes data to file using configured formatters and bins.
		 * \param data List of dictionaries representing data rows to save
		 * \return Status code (0 on success, non-zero on failure)
		 */
		int save(std::list<std::unordered_map<std::string, std::string>> data);
		
		/** Adds or updates the FileBin used for file output.
		 * \param b FileBin implementation for file operations
		 */
		void setFileBin(FileBin b);
		
		/** Adds or updates the FileFormatter used for data formatting.
		 * \param ff FileFormatter implementation for format conversion
		 */
		void setFileFormatter(FileFormatter ff);

	private:
		// contains vector of all used FileBin objects
		std::vector<std::unique_ptr<FileBin>> bins;
		// contains vectors of all used FileFormatters
		std::vector<std::unique_ptr<FileFormatter>> formatters;
};