# Docs

## Components

* Server
    1. Starts listening
    2. Spawns agent (provides id for authorization)
    3. Accepts connections
        * Client connected
            * Receive message
            * Client authorized?
                * Handle message
                * Go to receive message
            * Not authorized?
        * Handshake failed
            * Close connection with client
        * Handshake succeeded (authorization successful)
    4. On disconnect
        * Goto 2.

* Agent
    1. Initiate connect
    2. Connected
    3. Start authorization (use id, received as a command line argument)
        * Authorization failed
            * Agent process exits
        * Authorized
            * Start monitoring
                * Collect data
                    * Collected
                    * Build message
                    * Send message
    4. Disconnected
        * Agent process exits (later implement reconnect)

## CMake project layout

<https://github.com/PacktPublishing/Modern-CMake-for-Cpp/blob/main/.gitmodules>

* Structure

```txt
cpp-project-layout/
    cmake/
    src/
        app-x/
        cmake/
        module-x/
        CMakeLists.txt
    tests/
        app-x/
        module-x
    CMakeLists.txt
    CMakePresets.txt
```

* Also consider [this](https://gitlab.com/CLIUtils/modern-cmake/-/tree/master/examples/extended-project?ref_type=heads)
* <https://cliutils.gitlab.io/modern-cmake/chapters/basics/structure.html>

* Connected -> Authorized -> Disconnected

* EVENT: New connection
    * Has more then N connections?
        * YES:  Respond with "Refused to process more connections..." and LEAVE
    * Has `AUTHORIZED` agent?
        * YES:  Respond with "Already has an authorized agent" and LEAVE
        * NO:  Connection added into ActiveAgent list and agent goes into `CONNECTED` mode

* EVENT: Message received
    * Is ActiveAgent `AUTHORIZED`?
        * NO:  Is current message valid authorization message?
            * YES:  ActiveAgent goes into `AUTHORIZED` mode
            * NO:  Respond with "Authorization failed" and LEAVE
        * YES:  Process message

* EVENT: Disconnected or Error
    * Remove connection from ActiveAgent connections list
    * Was that a connection of ActiveAgent?
        * NO: LEAVE the scope
        * YES: ActiveAgent goes into Disconnected mode

-----------------------------------------------------------------------------

// agent messages
{"name": "auth", "message": {"username":"user", "token":"auth token"}}
{"name": "data", "message": {"tbd":"tbd"}}
{"name": "heartbeat", "message": {"up_time_ms":"epoch_time_mls","last_activity_time_ms":"22"}}

GetLastInputInfo(LASTINPUTINFO)

// server messages
{"status":0, "error":"description", "answer":{}}
{"status":0, "error":"description", "answer":{}}
{"status":0, "error":"description", "answer":{}}

-----------------------------------------------------------------------------

TEST(aaaaaa, dfdfdf)
{
    cereal::JSONOutputArchive ar(std::cout,
    cereal::JSONOutputArchive::Options::NoIndent());
    ProcessInfo pi;
    ar(cereal::make_nvp("pi", pi));
}

template <class Archive>
void serialize(Archive& archive, ProcessInfo& pi)
{
    //archive(pi.sha256);
    archive(cereal::make_nvp("sha", pi.sha256));
    //archive(pi.processPath, pi.sha256);
}

find_package(cereal CONFIG REQUIRED)
cereal::cereal

# target_link_libraries(main PRIVATE cereal::cereal)

//namespace glz::detail {
//template <>
//struct from_json<std::chrono::milliseconds>
//{
//    template <auto Opts>
//    static void op(std::chrono::milliseconds& mls, auto&&... args)
//    {
//        // Initialize a string_view with the appropriately lengthed buffer
//        // Alternatively, use a std::string for any size (but this will allocate)
//        std::string_view str = "0";
//        read<json>::op<Opts>(str, args...);
//        mls = std::stoi(str);
//    }
//};
//
//template <>
//struct to_json<std::chrono::milliseconds>
//{
//    template <auto Opts>
//    static void op(const std::chrono::milliseconds& mls, auto&&... args) noexcept
//    {
//        std::string str = std::to_string(mls.count());
//        write<json>::op<Opts>(str, args...);
//    }
//};
//
//template <>
//struct from_json<Rect>
//{
//    template <auto Opts>
//    static void op(Rect& rc, auto&&... args)
//    {
//        // Initialize a string_view with the appropriately lengthed buffer
//        // Alternatively, use a std::string for any size (but this will allocate)
//        std::string_view str = "";
//        read<json>::op<Opts>(str, args...);
//
//        std::ignore = rc;
//        //p = str;
//    }
//};
//
//template <>
//struct to_json<Rect>
//{
//    template <auto Opts>
//    static void op(const Rect& rc, auto&&... args) noexcept
//    {
//        char buffer[256] = R"({"lt": [0, 0], "wh": [5, 5]})";
//
//        write<json>::op<Opts>(buffer, args...);
//        std::ignore = rc;
//    }
//};
//
//} // namespace glz::detail
//
//template <>
//struct glz::meta<ProcessInfo>
//{
//    using T = ProcessInfo;
//    static constexpr auto value = glz::object(&T::processPath, &T::sha256);
//};

//void usage()
//{
//    Entry e;
//    Image i;
//    ProcessInfo pi {"aaaa", "sha"};
//    // WindowInfo wi;
//    std::chrono::milliseconds ms {10};
//    fs::path p("some/path");
//    WindowInfo wi;
//
//    std::string buffer {};
//    glz::write_json(wi, buffer);
//
//    std::cout << buffer << '\n';
//}

=============================================================>

/*
namespace {

bool criteriaMet(const Entry& entry, const std::string& s)
{
    std::ignore = s;
    return entry.processInfo.processPath.string().find("vlc") != std::string::npos;
}

} // namespace

TEST(FileSystemRepositoryTest, Checks)
{
    using Processes = std::map<fs::path, std::chrono::milliseconds>;
    using Titles = std::map<std::string, std::chrono::milliseconds>;

    size_t entriesQty = 0;
    Processes timeByProc;
    Titles titles;
    std::chrono::milliseconds totalDuration {};

    repo.queryEntries(
        filter,
        [&entriesQty, &totalDuration, &timeByProc, &titles](const Entry& entry) {
            totalDuration += entry.timestamp.duration;
            ++entriesQty;

            if (entriesQty % 10'000 == 0)
            {
                std::cout << "Entries processed so far: " << entriesQty << '\r';
                return true;
            }

            const auto title = lower(entry.windowInfo.title);
            if (criteriaMet(entry, title))
            {
                timeByProc[entry.processInfo.processPath.filename()] +=
                    entry.timestamp.duration;
                titles[entry.windowInfo.title] += entry.timestamp.duration;
            }

            return true; // return false to stop enumeration, true to continue
        });

    std::cout << "                                                            \r";
    std::cout << fmt::format("Total entires queried {}\n", entriesQty);
    std::cout << "Total duration: " << humanizeDuration(totalDuration, 3) << std::endl;


    std::vector<Processes::const_iterator> freqs;
    for (auto x = timeByProc.begin(); x != timeByProc.end(); ++x)
    {
        freqs.push_back(x);
    }
    std::sort(std::begin(freqs), std::end(freqs), [](const auto& lhs, const auto& rhs) {
        return lhs->second > rhs->second;
    });

    for (const auto& it : freqs)
    {
        std::cout << std::setw(10) << humanizeDuration(it->second, 2) << " : "
                  << it->first << '\n';
    }

    for (const auto& t : titles)
    {
        std::cout << t.first << " -> " << humanizeDuration(t.second, 2) << std::endl;
    }
}
*/

//class IFieldBuilder
//{
//public:
//    virtual ~IFieldBuilder() = default;
//
//    virtual std::string_view field(const Entry& entry) const = 0;
//    virtual std::string      value(const Entry& entry) const = 0;
//};

    //EXPECT_EQ(7, dd.rootNode()->nodesCount());
    //std::cout << "Total nodes count: " << dd.rootNode()->nodesCount() << std::endl;

/*
using FileCb = std::function<void (const std::filesystem::path& p)>;
void treeLoad(std::istream& is, FileCb cb)
{
    std::string s;
    std::wstring ws;
    std::filesystnamespace fs =em::path p;

    auto extract = [](const std::string& s) {
        size_t n = 0;
        auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), n);

        if (ec == std::errc())
        {
            return std::make_pair(n, std::string_view(ptr));
        }

        throw std::runtime_error("Corrupted data");
    };

    std::vector<std::wstring> files;
    while (is)
    {
        std::getline(is, s);
        auto [n, sv] = extract(s);

        while (n <= files.size())
        {
            files.pop_back();
        }

        str::s2ws(s, ws);
        files.push_back(ws);
        cb(p);
    }
}

# include <filesystem>

# include <charconv>

*/

// "Everything" only indexes file and folder names and generally takes a few seconds to build its database.

// A fresh install of Windows 10 (about 120,000 files) will take about 1 second to index.

// 1,000,000 files will take about 1 minute.

# Dups handling scenarios

1. Implement proper file/folder exclusion functionality

2. Compare 2 folders
    * Input `A`, `B`
    * Discovered `G` groups of duplicates `typeof(G[i])` is a list of strings
    * Ask pattern `K` to keep
        * If in the duplicate group G[i] keep pattern matches only one record, the rest will be deleted
    * Ask pattern `D` to delete
        * If in the duplicate group G[i] delete pattern matches not all records the matches will be deleted
    * Otherwise ask for confirmation in both cases

3.
