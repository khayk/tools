# Docs

## Components

* Server
    1. Starts listening
    2. Spawns agent (provides token for authorization)
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
    3. Start authorization (use token, received as a command line argument)
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

=============================================================>



*/

// "Everything" only indexes file and folder names and generally takes a few seconds to build its database.

// A fresh install of Windows 10 (about 120,000 files) will take about 1 second to index.

// 1,000,000 files will take about 1 minute.

