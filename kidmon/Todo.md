# Todo

## Open Issues

* Figure out why u8'*' tests are failing on linux
* Add version to the app

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
