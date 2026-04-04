#pragma once

/**
 * \file DataSaver.hpp
 * \author Shamar Pennant
 * \brief Coordinates FileFormatter and FileBin to persist structured data to files.
 *
 * Maintains collections of FileBin and FileFormatter objects to support saving
 * list<Dict<string,string>> data in multiple file formats simultaneously.
 */

#include "FileBin.hpp"
#include "FileFormatter.hpp"
#include <vector>
#include <memory>
#include "File_ID.hpp"

/**
 * \class DataSaver
 * \brief DataSaver class declaration.
 */
class DataSaver {
    public:
        /**
         * Constructs a DataSaver with an initial FileBin and FileFormatter.
         * \param b  The FileBin implementation for file output.
         * \param ff The FileFormatter implementation for data formatting.
         */
        DataSaver(FileBin b, FileFormatter ff);

        ~DataSaver();

        /**
         * Formats and writes data to file using the configured formatters and bins.
         * \param data List of row dictionaries to save.
         * \return 0 on success, non-zero on failure.
         */
        int save(std::list<std::unordered_map<std::string, std::string>> data);

        /**
         * Adds or replaces the FileBin used for file output.
         * \param b New FileBin implementation.
         */
        void setFileBin(FileBin b);

        /**
         * Adds or replaces the FileFormatter used for data formatting.
         * \param ff New FileFormatter implementation.
         */
        void setFileFormatter(FileFormatter ff);

    private:
        std::vector<std::unique_ptr<FileBin>>        bins;       ///< Registered file output handlers.
        std::vector<std::unique_ptr<FileFormatter>>  formatters; ///< Registered data formatters.
};
