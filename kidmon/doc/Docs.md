# Docs

@todo: this file might be out of date. update when time comes.

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

* Also consider [this](https://gitlab.com/CLIUtils/modern-cmake/-/tree/master/examples/extended-project?ref_type=heads)

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

*/

// "Everything" only indexes file and folder names and generally takes a few seconds to build its database.
// A fresh install of Windows 10 (about 120,000 files) will take about 1 second to index.
// 1,000,000 files will take about 1 minute.

