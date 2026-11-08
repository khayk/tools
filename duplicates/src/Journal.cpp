#include <duplicates/Journal.h>
#include <core/utils/File.h>
#include <core/utils/FmtExt.h>
#include <core/utils/Str.h>

#include <spdlog/spdlog.h>

#include <format>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <system_error>

namespace tools::dups {
namespace {

constexpr char SEPARATOR = '|';

fs::path utf8ToPath(std::string_view utf8)
{
    return {core::str::stou8(utf8)};
}

} // namespace

std::string formatEntry(const JournalEntry& entry)
{
    std::ostringstream os;
    os << std::quoted(core::file::path2s(entry.original)) << SEPARATOR
       << std::quoted(core::file::path2s(entry.backup));
    return std::move(os).str();
}

std::optional<JournalEntry> parseEntry(std::string_view line)
{
    std::istringstream is {std::string(line)};
    std::string original;
    std::string backup;
    char sep {};

    // std::quoted on input silently accepts an unquoted word, so insist on the
    // opening quote of each field to reject lines that are not in our format.
    if (is.peek() != '"' || !(is >> std::quoted(original)) || !is.get(sep) ||
        sep != SEPARATOR || is.peek() != '"' || !(is >> std::quoted(backup)))
    {
        return std::nullopt;
    }

    // Only whitespace may follow the second field.
    is >> std::ws;
    if (!is.eof() || original.empty() || backup.empty())
    {
        return std::nullopt;
    }

    return JournalEntry {utf8ToPath(original), utf8ToPath(backup)};
}

std::vector<JournalEntry> readJournal(const fs::path& file)
{
    // A plain stream rather than core::file::readLines: the journal may still be
    // open by a JournalWriter, and the memory-mapped reader requests a share mode
    // on Windows that conflicts with an open writer.
    std::ifstream in(file, std::ios::binary);
    if (!in)
    {
        throw std::system_error(
            std::make_error_code(std::errc::no_such_file_or_directory),
            std::format("Unable to open file: {}", file));
    }

    std::vector<JournalEntry> entries;
    std::string line;
    size_t lineNo = 0;

    while (std::getline(in, line))
    {
        ++lineNo;

        if (line.find_first_not_of(" \t\r") == std::string::npos)
        {
            continue;
        }

        if (auto entry = parseEntry(line))
        {
            entries.push_back(std::move(*entry));
        }
        else
        {
            spdlog::warn("Skipping malformed journal line {} in: {}", lineNo, file);
        }
    }

    return entries;
}

JournalWriter::JournalWriter(fs::path path)
    : path_(std::move(path))
{
}

void JournalWriter::append(const JournalEntry& entry)
{
    if (!out_.is_open())
    {
        // Binary mode keeps '\n' line endings on every OS, so journals written on
        // one platform read back identically on another.
        out_.open(path_, std::ios::out | std::ios::app | std::ios::binary);

        if (!out_)
        {
            throw std::system_error(
                std::make_error_code(std::errc::no_such_file_or_directory),
                std::format("Unable to open file: {}", path_));
        }
    }

    // Flush per entry: the journal is the only record of where backups went, so
    // an entry must reach the disk before the file it describes is moved.
    out_ << formatEntry(entry) << '\n' << std::flush;

    if (!out_)
    {
        throw std::system_error(std::make_error_code(std::errc::io_error),
                                std::format("Unable to write to file: {}", path_));
    }
}

const fs::path& JournalWriter::path() const noexcept
{
    return path_;
}

} // namespace tools::dups
