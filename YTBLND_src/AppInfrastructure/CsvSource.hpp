/**
 * \file CsvSource.cpp
 * \author Shamar Pennant
 * \brief CSV file source/reader implementation
 * 
 * Implements the FileSource interface for reading CSV files.
 * Reads file contents line-by-line and returns as list<string>.
 */

#pragma once

#include "FileSource.hpp"

class CsvSource : public FileSource {
	public:
		CsvSource(std::string src);
		~CsvSource() override = default;

		std::list<std::string> read() override;
		void setSource(std::string src) override;
		int getSourceId() override;
	private:
	std::string src;
	int source_id;
};