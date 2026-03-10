#pragma once
#include <string>
#include <iostream>
#include <fstream>
#include <list>

/**
 * \author Shamar Pennant
 * \brief Reads a desired file format into a single string 
 */
class FileSource {
	public:
		virtual ~FileSource() = default;

		/** reads the set path to file on instantiation. 
		Returning a string of raw file data */
		virtual std::list<std::string> read() = 0;

		/** Copies the given pramater to a local version 
		 * for read() to use later */
		virtual void setSource(std::string src) = 0;

		/** Copies given id to source_id */
		virtual void setSourceId(int id) = 0;
		virtual int getSourceId() = 0;

	protected:
		FileSource() = default;
};