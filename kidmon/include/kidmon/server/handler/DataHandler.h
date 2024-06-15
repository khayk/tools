#pragma once

#include "MsgHandler.h"
#include <kidmon/data/Types.h>
#include <filesystem>

namespace fs = std::filesystem;

//
// * Entry          // full representation of the event
// * Statistics     // statistical data about multiple entries (part of report)
//
// * Report         // summary of the collection of an event based on a certain criteria
// * ReportBuilder  // interface for producing a report
//
// * IDataStorage   // interface for event storage
//      * DatabaseStorage
//      * FileSystemStorage
//      * MemoryStorage


class IDataStorage
{
public:
    virtual ~IDataStorage() = default;

    virtual void add(const Entry& entry) = 0;
};

class FileSystemStorage : public IDataStorage
{
public:
    explicit FileSystemStorage(fs::path reportsDir);
    ~FileSystemStorage();

    void add(const Entry& entry) override;

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

class DataHandler : public MsgHandler
{
    IDataStorage& storage_;
    std::string buffer_;

public:
    explicit DataHandler(IDataStorage& storage);

    bool handle(const nlohmann::json& payload,
                nlohmann::json& answer,
                std::string& error) override;
};
