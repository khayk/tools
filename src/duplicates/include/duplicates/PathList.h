#include <unordered_set>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

namespace tools::dups {

using PathsVec = std::vector<fs::path>;
using PathsSet = std::unordered_set<fs::path>;

class PathList
{
public:
    PathList() = default;
    PathList(fs::path file, bool saveWhenDone = true);
    ~PathList();

    const PathsSet& files() const;

    bool contains(const fs::path& path) const;

    bool empty() const noexcept;

    size_t size() const noexcept;

    void add(const fs::path& path);

    void add(const PathsVec& paths);

private:
    /**
     * @brief Saves the files inside `filePath_` file
     */
    void save() const;

    /**
     * @brief Loads the files from the `filePath_` file
     */
    void load();

    fs::path filePath_;
    PathsSet paths_;
    bool saveWhenDone_ {false};
};

} // namespace tools::dups
