#pragma once

#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;

namespace tools::dups {

/**
 * @brief Name of the journal file inside each run's backup directory.
 */
inline constexpr std::string_view JOURNAL_FILE_NAME = "deleted_files.log";

/**
 * @brief One deletion recorded in a run's journal: where the file lived and where
 *        its backup was moved to. Both paths are absolute.
 */
struct JournalEntry
{
    fs::path original;
    fs::path backup;

    bool operator==(const JournalEntry&) const = default;
};

/**
 * @brief Serialize an entry as a single journal line (without the newline).
 *
 * The format is `"<original>"|"<backup>"`: each path is UTF-8 encoded and quoted
 * with std::quoted, so separators, spaces, quotes and backslashes in the paths
 * round-trip exactly.
 */
std::string formatEntry(const JournalEntry& entry);

/**
 * @brief Parse a line produced by formatEntry.
 *
 * Trailing whitespace (including a '\r' left by CRLF line endings) is tolerated.
 *
 * @return The entry, or std::nullopt if the line is malformed.
 */
std::optional<JournalEntry> parseEntry(std::string_view line);

/**
 * @brief Read all entries of a journal file, in the order they were written.
 *        Empty lines are ignored; malformed lines are skipped with a warning.
 *
 * @throw std::exception if the file cannot be read.
 */
std::vector<JournalEntry> readJournal(const fs::path& file);

/**
 * @brief Appends entries to a journal file. The file is created lazily on the
 *        first append, so a run without deletions leaves no journal behind.
 *        Every entry is flushed immediately so it survives a crash of the app.
 */
class JournalWriter
{
    fs::path path_;
    std::ofstream out_;

public:
    explicit JournalWriter(fs::path path);

    /**
     * @throw std::system_error if the journal file cannot be opened or written.
     */
    void append(const JournalEntry& entry);

    const fs::path& path() const noexcept;
};

} // namespace tools::dups
