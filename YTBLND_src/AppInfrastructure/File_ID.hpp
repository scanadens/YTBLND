/**
 * \file File_ID.hpp
 * \author Shamar Pennant
 * \brief File format ID constants
 * 
 * Defines static integer constants mapping file types to unique identifiers.
 * Used to validate that FileSource and Parser objects are compatible.
 */
#pragma once

/// File format ID constants for validating source-parser compatibility.
class File_ID {
	public:
	static constexpr int CSV = 1;
	static constexpr int HTML = 2;
	static constexpr int JSON = 3;
};
