#include <unordered_set>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

namespace tools::dups {

using PathsVec = std::vector<fs::path>;
using PathsSet = std::unordered_set<fs::path>;

class PathsImpl
{
    PathsSet paths_;

public:
    PathsImpl() = default;
    PathsImpl(const PathsVec& paths);
    PathsImpl(PathsSet paths) noexcept;

    PathsSet& paths();

    const PathsSet& paths() const;

    bool contains(const fs::path& path) const;

    bool empty() const noexcept;

    size_t size() const noexcept;

    void add(const fs::path& path);

    void add(const PathsVec& paths);
};


class PathsPersister
{
    PathsSet& paths_;
    fs::path filePath_;
    bool saveWhenDone_ {false};

public:
    PathsPersister(PathsSet& paths, fs::path filePath, bool saveWhenDone = true);
    ~PathsPersister();

    PathsPersister(const PathsPersister&) = delete;
    PathsPersister(PathsPersister&&) = delete;
    PathsPersister& operator=(const PathsPersister&) = delete;
    PathsPersister& operator=(PathsPersister&&) = delete;

private:
    /**
     * @brief Saves the files inside `filePath_` file
     */
    void save() const;

    /**
     * @brief Loads the files from the `filePath_` file
     */
    void load();
};


template <auto EnumPurpose>
class Paths : public PathsImpl
{
    static_assert(std::is_enum_v<decltype(EnumPurpose)>,
                  "StrongType must be instantiated with scoped enum (enum class)");

public:
    Paths() = default;
    Paths(const PathsVec& paths)
        : PathsImpl(paths)
    {
    }
    Paths(const PathsSet& paths)
        : PathsImpl(paths)
    {
    }
};

} // namespace tools::dups
