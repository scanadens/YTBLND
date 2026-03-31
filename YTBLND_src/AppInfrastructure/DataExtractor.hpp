#pragma once

#include <list>
#include <memory>
#include <string>
#include <unordered_map>

#include "FileSource.hpp"
#include "Parser.hpp"
#include "File_ID.hpp"

/**
 * \file DataExtractor.hpp
 * \author Shamar Pennant
 * \brief Extracts raw file data and parses it into structured format
 * 
 * Coordinates file reading (via FileSource) and data parsing (via Parser) to
 * produce a consistent list<Dict<str,str>> format. Validates that FileSource
 * and Parser objects have matching file format IDs before processing.
 * 
 * Example usage:
 * \code
 * auto source = std::make_unique<CsvSource>("data.csv");
 * auto parser = std::make_unique<CsvParser>();
 * DataExtractor extractor(std::move(source), std::move(parser));
 * auto data = extractor.extract();
 * \endcode
 */
/// Coordinates FileSource and Parser to extract and parse file data.
class DataExtractor {
	public:
		/** Constructs a DataExtractor with FileSource and Parser.
		 * \param src Unique pointer to FileSource implementation (must be configured)
		 * \param p Unique pointer to Parser implementation (must be configured)
		 * \note FileSource and Parser must have matching format IDs
		 */
		DataExtractor(std::unique_ptr<FileSource> src, std::unique_ptr<Parser> p);
		
		/** Destructor. Cleans up owned FileSource and Parser objects. */
		~DataExtractor();

		/** Extracts and parses file data using configured FileSource and Parser.
		 * 
		 * Reads raw data from FileSource, validates format IDs match, feeds data
		 * to Parser, and returns structured format.
		 * 
		 * \return List of dictionaries where each dict represents a data row
		 * \throws Calls exit(1) if FileSource and Parser format IDs don't match
		 * \throws Calls exit(1) if pointers are null or validation fails
		 */
		std::list<std::unordered_map<std::string, std::string>> extract();
		
		/** Replaces the current FileSource with a new one.
		 * \param src Unique pointer to new FileSource implementation
		 */
		void setSource(std::unique_ptr<FileSource> src);
		
		/** Replaces the current Parser with a new one.
		 * \param p Unique pointer to new Parser implementation
		 */
		void setParser(std::unique_ptr<Parser> p);

	private:
		std::unique_ptr<FileSource> src_format;
		std::unique_ptr<Parser> par_format;
};