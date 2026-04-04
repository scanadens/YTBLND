/**
 * \file JsonUtils.hpp
 * \brief Utility functions for JSON string formatting and serialization.
 * \author Shamar Pennant
 * 
 * Provides helper functions for escaping special characters in JSON strings,
 * quoting values, and converting timestamps to ISO8601 format.
 */

#ifndef MODEL_JSON_UTILS_H
#define MODEL_JSON_UTILS_H

#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>

/**
 * \namespace ModelJson
 * \brief JSON formatting utilities for model serialization.
 */
namespace ModelJson {


/**
 * Escapes special characters in a string for use in JSON values.
 * 
 * Converts escape sequences such as quotes, backslashes, newlines, tabs,
 * and control characters into their JSON-safe representations. Characters
 * below 0x20 are converted to Unicode escape sequences.
 *
 * \param input The input string to escape.
 * \return A new string with all special characters properly escaped for JSON.
 */
inline std::string escape(const std::string& input) {
    std::ostringstream out;
    for (unsigned char c : input) {
        switch (c) {
            case '"': out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            case '\b': out << "\\b"; break;
            case '\f': out << "\\f"; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default:
                if (c < 0x20) {
                    out << "\\u"
                        << std::hex << std::uppercase << std::setw(4)
                        << std::setfill('0') << static_cast<int>(c)
                        << std::dec;
                } else {
                    out << static_cast<char>(c);
                }
        }
    }
    return out.str();
}

/**
 * Wraps a string in double quotes after escaping special characters.
 *
 * Combines escape() and quoting to produce a JSON-safe string value.
 * The returned string includes the surrounding double quotes.
 *
 * \param input The input string to quote and escape.
 * \return A JSON-safe quoted string (e.g., "hello world").
 */
inline std::string quote(const std::string& input) {
    return std::string("\"") + escape(input) + "\"";
}

/**
 * Converts a UNIX timestamp to an ISO 8601 formatted string.
 *
 * Formats the given timestamp (seconds since epoch) as an ISO 8601 string
 * in UTC timezone. The format is: YYYY-MM-DDTHH:MM:SSZ
 *
 * \param timestamp UNIX timestamp (seconds since epoch).
 * \return \code ISO 8601 \endcode formatted timestamp string in UTC (e.g., \c "2026-04-03T12:30:45Z").
 */
inline std::string toIso8601(std::time_t timestamp) {
    std::tm utc{};
#if defined(_WIN32)
    gmtime_s(&utc, &timestamp);
#else
    gmtime_r(&timestamp, &utc);
#endif

    std::ostringstream out;
    out << std::put_time(&utc, "%Y-%m-%dT%H:%M:%SZ");
    return out.str();
}

} // namespace ModelJson

#endif
