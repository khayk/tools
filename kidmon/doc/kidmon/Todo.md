# Todo

## Open Issues

* Add version to the app
* Slightly alter project organization to be something like this (Claude comms)

and consider this

```
repo/
├── libs/
│   ├── core/
│   ├── network/
│   └── serialization/
├── apps/
│   ├── server/
│   └── client/
└── tests/
    └── integration/
```

* I guess this is a reasonable restructuring proposal for kidmon
```
apps/kidmon/
├── CMakeLists.txt
├── src/
│   ├── Main.cpp                # parses --mode, dispatches
│   ├── common/
│   │   ├── AppBase.cpp         # shared: logging, config, signal handling
│   │   └── AppBase.h
│   ├── service/
│   │   ├── ServiceApp.cpp      # watchdog, data storage, TCP server
│   │   ├── AgentManager.cpp    # spawn/monitor/respawn agent process
│   │   └── handler/
│   │       ├── AuthorizationHandler.cpp
│   │       └── DataHandler.cpp
│   ├── agent/
│   │    ├── AgentApp.cpp        # collection loop, sends data to service
│   │    ├── Collector.cpp
│   │    └── os/
│   │        ├── linux/
│   │        ├── mac/
│   │        └── win/
│   └── internal/           # testable logic lives here
└── tests/
    ├── CMakeLists.txt
    ├
tests/
├── integration
```

## In Progress

*

## Done

* Invalid json causes server to stop
    * Consider catching exception in the services, as an invalid json which causes an exception eventually stops the server
    * It happened when I have run tests while server is running
    * Logs
        * kidmon-server-2024-08-31.log
        * kidmon-server-2024-08-08.log
* Server and agent should have their own config
* Figure out why u8'*' tests are failing on linux
