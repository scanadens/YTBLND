#pragma once

#include "FileBin.hpp"
#include "FileFormatter.hpp"
#include <vector>
#include <memory>
#include "File_ID.hpp"

/**
 * \author Shamar Pennant
 * \brief Saves run time data as the desired file format
 */
class DataSaver {
	public:
		DataSaver(FileBin b, FileFormatter ff);
		~DataSaver();

		// uses data members to save their contents as the specified file format
		int save(std::list<std::unordered_map<std::string, std::string>> data);
		/* copies the given file bin to the FileBin vector (unless it has the 
		same bin object already)
		*/
		void setFileBin(FileBin b);
		/* copies the given file formatter to the FileFormatter vector 
		(unless it has the FileFormatter object already)
		*/
		void setFileFormatter(FileFormatter ff);

	private:
		// contains vector of all used FileBin objects
		std::vector<std::unique_ptr<FileBin>> bins;
		// contains vectors of all used FileFormatters
		std::vector<std::unique_ptr<FileFormatter>> formatters;
};