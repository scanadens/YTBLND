/**
 * \file CsvSource.hpp
 * \author Shamar Pennant
 * \brief CSV file reader implementing the FileSource interface.
 *
 * Reads a CSV file line-by-line and returns each line as a string in
 * a list, ready to be handed to a CsvParser.
 */

#pragma once

#include "FileSource.hpp"

/// CSV file reader implementing the FileSource interface.
class CsvSource : public FileSource {
    public:
        /**
         * Constructs a CsvSource configured for the given file path.
         * \param src Path to the CSV file to read.
         */
        CsvSource(std::string src);
        ~CsvSource() override = default;

        /**
         * Reads the configured CSV file line-by-line.
         * \return List of raw text lines from the file.
         */
        std::list<std::string> read() override;

        /**
         * Changes the file path used by read().
         * \param src New file path.
         */
        void setSource(std::string src) override;

        /// \return File_ID::CSV
        int getSourceId() override;

    private:
        std::string src;
        int source_id;
};
