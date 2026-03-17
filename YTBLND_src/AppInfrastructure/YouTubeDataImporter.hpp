#pragma once

#include "FileSource.hpp"
#include "Parser.hpp"
#include "File_ID.hpp"
#include "../ModelLayer/Video.h"

/**
 * \file YouTubeDataImporter.hpp
 * \author Shamar Pennant
 * \brief Pipeline structure for accessing YouTube specific data
 * 
 * Coordinates specialized implementations of FileSource and Parser to 
 * produce a list of Video objects. Importing data based on csv (for 
 * playlists) or html (for watch history data) file formats 
 */