#pragma once

#include <string_view>
#include <filesystem>
#include <system_error>
#include <functional>
#include <regex>

namespace fs = std::filesystem;

namespace file {

void write(const fs::path& file, std::string_view data);

void write(const fs::path& file, const char* data, size_t size);

void append(const fs::path& file, std::string_view data);

void append(const fs::path& file, const char* data, size_t size);

/**
 * @brief Read the content of the file into 'data' string
 *
 * @param file Path to the file.
 * @param data An object to be filled with the file content.
 *
 * @return true if the operation completed successfully, otherwise false
 */
bool read(const fs::path& file, std::string& data, std::error_code& ec);

/**
 * @brief Callback for readLines.
 *        To stop enumeration immediately the callback should return false,
 * otherwise true, in that case enumeration will be stop when there is more data
 * to be enumerated
 */
using LineCb = std::function<bool(const std::string&)>;

/**
 * @brief Read the content of the file line by line
 *
 * @param file Path to the file.
 * @param cb A functor to be invoked for each line
 *
 * @throw Throw an exception on error
 */
void readLines(const fs::path& file, const LineCb& cb);

std::string path2s(const fs::path& path);


/**
 * @brief  Constructs path with unique name, taking into account provided prefix and
 * temp directory. It will create path object only, the underlying path will not be
 * created on file system.
 *
 * The resulting path will be equal to:
 *
 *      `tempDir/suffix-PID-******`  if suffix provided
 *      `tempDir/tmp-PID-******`     if suffix is empty
 *
 * Where:
 *
 *      `*`    is an english lowercase character
 *      `PID`  is the process id where the function is being called
 *
 * @note  If `tempDir` is empty system temp directory path will be used.
 *
 * @param namePrefix
 * @param tempDir
 *
 * @return  The constructed path object
 */
fs::path constructTempPath(std::string_view namePrefix = "",
                           const fs::path& tempDir = fs::path());


class Path
{
private:
    fs::path path_;
    bool owner_ {true};

protected:
    Path() noexcept;
    explicit Path(fs::path p) noexcept;

public:
    [[nodiscard]] const fs::path& path() const noexcept;

    /**
     * @brief Check if the path is owned by this object
     */
    [[nodiscard]] bool isOwner() const noexcept;

    /**
     * @brief Drop ownership of the path
     */
    void dropOwnership() noexcept;

    /**
     * @brief Take ownership for removing given path on destruction
     *
     * @param newPath  The new path
     */
    void takeOwnership(const fs::path& newPath);


    Path(const Path&) = delete;
    Path(Path&&) noexcept = default;
    Path& operator=(const Path&) = delete;
    Path& operator=(Path&&) noexcept = default;
};


class ScopedDir : public Path
{
public:
    ScopedDir() = default;
    explicit ScopedDir(fs::path dirPath);
    ~ScopedDir();
};


/**
 * @brief  See TempFile. On destruction recursively deletes unique temporary
 * directory held by this object
 */
class TempDir : public ScopedDir
{
public:
    enum class CreateMode : std::uint8_t
    {
        // Temp directories will be created automatically
        Auto,

        // Directory will NOT be created automatically
        Manual
    };

    TempDir(std::string_view namePrefix = "",
            const CreateMode createMode = CreateMode::Auto,
            const fs::path& tempDir = fs::path());
};


using PathCallback = std::function<void(const fs::path&, const std::error_code&)>;

/**
 * @brief Recursively list files in the given directory
 *
 * @param dir Input directory
 * @param exclusionPatterns Director and file patterns to be excluded from the
 * enumeration
 * @param cb Callback for emitting results
 */
void enumFilesRecursive(const fs::path& dir,
                        const std::vector<std::regex>& exclusionPatterns,
                        const PathCallback& cb);

/**
 * @brief Open the directory with the default file navigator. If the input is a regular
 *        file the parent directory will be navigated.
 *
 * @param path The input path
 */
void openDirectory(const fs::path& path);

/**
 * @brief Opens the directory containing the specified file and select the file
 *
 * @param file The input file
 */
void navigateFile(const fs::path& file);

} // namespace file