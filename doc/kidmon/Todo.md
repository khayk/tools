# Todo

## Open Issues

* Add version to the app

## In Progress

* Structure project as displayed below

```txt
project/
    cmake/
        include/
        module/
        script/
    src/
        app1/
            include/
                a_file.h
            a.file.cpp

            lib3/
            test/
        app2
        lib1
            include
                lib1/

        lib2
    doc/
    extern/
    test/
```

```txt
    core
    duplicate
    kidmon
    kidmon-reports
```

## Done

* Invalid json causes server to stop
    * Consider catching exception in the services, as an invalid json which causes an exception eventually stops the server
    * It happened when I have run tests while server is running
    * Logs
        * kidmon-server-2024-08-31.log
        * kidmon-server-2024-08-08.log
* Server and agent should have their own config
* Figure out why u8'*' tests are failing on linux
